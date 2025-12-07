#include "server/sync_service.h"
#include "common/logger.h"
#include <chrono>

namespace dropboxlite {

SyncServiceImpl::SyncServiceImpl(const std::string& storage_root) {
    storage_ = std::make_unique<StorageManager>(storage_root);
    storage_->initialize();
    conflict_resolver_ = std::make_unique<ConflictResolver>();
}

grpc::Status SyncServiceImpl::Sync(grpc::ServerContext* context,
                                   const SyncRequest* request,
                                   SyncResponse* response) {
    LOG_INFO("Sync request from client: " + request->client_id());
    
    // Get server files for this client
    auto server_files = storage_->listFiles(request->client_id());
    
    // Compute changes
    std::vector<FileMetadata> local_files(
        request->local_files().begin(),
        request->local_files().end()
    );
    
    auto changes = computeChanges(request->client_id(), local_files, request->last_sync_time());
    
    for (const auto& change : changes) {
        *response->add_changes() = change;
    }
    
    response->set_server_time(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    
    return grpc::Status::OK;
}

grpc::Status SyncServiceImpl::UploadFile(grpc::ServerContext* context,
                                        grpc::ServerReader<UploadChunkRequest>* reader,
                                        UploadChunkResponse* response) {
    UploadChunkRequest request;
    std::string client_id;
    std::string filepath;
    int32_t total_chunks = 0;
    int32_t chunks_received = 0;
    
    while (reader->Read(&request)) {
        if (client_id.empty()) {
            client_id = request.client_id();
            filepath = request.file_path();
            total_chunks = request.total_chunks();
        }
        
        const auto& chunk = request.chunk();
        std::vector<uint8_t> data(chunk.data().begin(), chunk.data().end());
        
        if (!storage_->storeChunk(client_id, filepath, chunk.index(), data, chunk.hash())) {
            response->set_success(false);
            response->set_message("Failed to store chunk");
            return grpc::Status::OK;
        }
        
        chunks_received++;
    }
    
    // Finalize file
    if (storage_->finalizeFile(client_id, filepath, total_chunks)) {
        response->set_success(true);
        response->set_chunks_received(chunks_received);
        LOG_INFO("File uploaded successfully: " + filepath);
    } else {
        response->set_success(false);
        response->set_message("Failed to finalize file");
    }
    
    return grpc::Status::OK;
}

grpc::Status SyncServiceImpl::DownloadFile(grpc::ServerContext* context,
                                          const DownloadRequest* request,
                                          grpc::ServerWriter<DownloadResponse>* writer) {
    LOG_INFO("Download request: " + request->file_path());
    
    auto metadata = storage_->getFileMetadata(request->client_id(), request->file_path());
    if (!metadata) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found");
    }
    
    // Get file chunks
    // This is simplified - in production you'd stream actual chunks
    DownloadResponse response;
    response.set_is_last(true);
    
    writer->Write(response);
    
    return grpc::Status::OK;
}

grpc::Status SyncServiceImpl::ResolveConflict(grpc::ServerContext* context,
                                             const ConflictResolutionRequest* request,
                                             ConflictResolutionResponse* response) {
    LOG_INFO("Conflict resolution for: " + request->file_path());
    
    response->set_success(true);
    response->set_resolved_path(request->file_path());
    
    return grpc::Status::OK;
}

grpc::Status SyncServiceImpl::StreamSync(grpc::ServerContext* context,
                                        grpc::ServerReaderWriter<FileChange, FileChange>* stream) {
    FileChange change;
    while (stream->Read(&change)) {
        LOG_INFO("Received change: " + change.path());
        // Process and broadcast to other clients
    }
    
    return grpc::Status::OK;
}

grpc::Status SyncServiceImpl::Heartbeat(grpc::ServerContext* context,
                                       const HeartbeatRequest* request,
                                       HeartbeatResponse* response) {
    response->set_server_timestamp(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    
    return grpc::Status::OK;
}

std::vector<FileChange> SyncServiceImpl::computeChanges(
    const std::string& client_id,
    const std::vector<FileMetadata>& local_files,
    int64_t last_sync_time) {
    
    std::vector<FileChange> changes;
    auto server_files = storage_->listFiles(client_id);
    
    // Find files that exist on server but not on client
    for (const auto& server_file : server_files) {
        bool found = false;
        for (const auto& local_file : local_files) {
            if (local_file.path() == server_file.path) {
                found = true;
                
                // Check for conflicts
                if (local_file.hash() != server_file.hash) {
                    FileChange change;
                    change.set_path(server_file.path);
                    change.set_type(FileChange::MODIFIED);
                    changes.push_back(change);
                }
                break;
            }
        }
        
        if (!found && server_file.modified_time > last_sync_time) {
            FileChange change;
            change.set_path(server_file.path);
            change.set_type(FileChange::CREATED);
            changes.push_back(change);
        }
    }
    
    return changes;
}

bool SyncServiceImpl::detectConflict(const FileMetadata& local, const FileRecord& server) {
    return local.hash() != server.hash && 
           local.version() > 0 && 
           server.version > 0;
}

} // namespace dropboxlite
