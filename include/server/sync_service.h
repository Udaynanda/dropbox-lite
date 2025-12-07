#pragma once

#include "server/storage_manager.h"
#include "core/conflict_resolver.h"
#include "sync.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

namespace dropboxlite {

class SyncServiceImpl final : public SyncService::Service {
public:
    explicit SyncServiceImpl(const std::string& storage_root);
    
    // Sync RPC
    grpc::Status Sync(grpc::ServerContext* context,
                     const SyncRequest* request,
                     SyncResponse* response) override;
    
    // Upload file RPC (client streaming)
    grpc::Status UploadFile(grpc::ServerContext* context,
                           grpc::ServerReader<UploadChunkRequest>* reader,
                           UploadChunkResponse* response) override;
    
    // Download file RPC (server streaming)
    grpc::Status DownloadFile(grpc::ServerContext* context,
                             const DownloadRequest* request,
                             grpc::ServerWriter<DownloadResponse>* writer) override;
    
    // Resolve conflict RPC
    grpc::Status ResolveConflict(grpc::ServerContext* context,
                                const ConflictResolutionRequest* request,
                                ConflictResolutionResponse* response) override;
    
    // Bidirectional streaming sync
    grpc::Status StreamSync(grpc::ServerContext* context,
                           grpc::ServerReaderWriter<FileChange, FileChange>* stream) override;
    
    // Heartbeat RPC
    grpc::Status Heartbeat(grpc::ServerContext* context,
                          const HeartbeatRequest* request,
                          HeartbeatResponse* response) override;
    
private:
    std::unique_ptr<StorageManager> storage_;
    std::unique_ptr<ConflictResolver> conflict_resolver_;
    
    // Helper methods
    std::vector<FileChange> computeChanges(const std::string& client_id,
                                          const std::vector<FileMetadata>& local_files,
                                          int64_t last_sync_time);
    
    bool detectConflict(const FileMetadata& local, const FileRecord& server);
};

} // namespace dropboxlite
