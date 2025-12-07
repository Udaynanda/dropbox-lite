#include "server/sync_service.h"
#include "common/logger.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signalHandler(int signal) {
    running = false;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <storage_root> <port>\n";
        std::cerr << "Example: " << argv[0] << " ./storage 50051\n";
        return 1;
    }
    
    std::string storage_root = argv[1];
    std::string port = argv[2];
    std::string server_address = "0.0.0.0:" + port;
    
    // Setup logging
    dropboxlite::Logger::instance().setLevel(dropboxlite::LogLevel::INFO);
    dropboxlite::Logger::instance().setLogFile("dropbox_server.log");
    
    LOG_INFO("Starting Dropbox Lite Server");
    LOG_INFO("Storage root: " + storage_root);
    LOG_INFO("Listening on: " + server_address);
    
    // Create service
    dropboxlite::SyncServiceImpl service(storage_root);
    
    // Build server
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    
    if (!server) {
        LOG_ERROR("Failed to start server");
        return 1;
    }
    
    LOG_INFO("Server started successfully");
    
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Wait for shutdown
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LOG_INFO("Shutting down server...");
    server->Shutdown();
    
    return 0;
}
