#include <iostream>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <future>

#include <ael/event_loop.h>
#include <ael/stream_buffer.h>
#include <ael/stream_listener.h>

#include <ael/openssl/ssl_stream_buffer_filter.h>

#include "elapsed.h"

using namespace std;
using namespace ael;

static Elapsed elapsed;

#define TCP_CONNECT 2
#define SSL_FIRST_CONNECT 1
#define SSL_SECOND_CONNECT 0

class PingServer : public NewConnectionHandler, public StreamBufferHandler, public enable_shared_from_this<StreamBufferHandler> {
public:
    PingServer(shared_ptr<EventLoop> event_loop) : event_loop_(event_loop) {
        ssl_ctx_ = SSL_CTX_new(SSLv23_server_method());

	    if (SSL_CTX_use_certificate_file(ssl_ctx_, "fake1.crt", SSL_FILETYPE_PEM) <= 0) {
	    	throw "failed to load certificate";
	    }

	    if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, "fake1.key", SSL_FILETYPE_PEM) <= 0 ) {
	    	throw "failed to load key";
	    }
    }

    virtual ~PingServer() {
        SSL_CTX_free(ssl_ctx_);
    }

    void HandleNewConnection(Handle handle) override {
        cout << elapsed << "new connection" << endl;
        auto stream_buffer = StreamBuffer::CreateForServer(shared_from_this(), handle);
        streams_buffers_[stream_buffer] = TCP_CONNECT;
        event_loop_->Attach(stream_buffer);
    }

    void HandleData(std::shared_ptr<StreamBuffer> stream_buffer, const std::shared_ptr<const DataView> &data_view) override {
        string data;
        data_view->AppendToString(data);

        if (data.find("ping") != string::npos) {
            stream_buffer->Write(string("pong"));
        }
    }

	void HandleConnected(std::shared_ptr<StreamBuffer> stream_buffer) override { 
        int &state = streams_buffers_[stream_buffer];
        SSL *ssl;
        shared_ptr<SSLStreamBufferFilter> ssl_filter;

        switch(state) {
        case TCP_CONNECT:
            cout << elapsed << "connected TCP - upgrading to SSL" << endl;
            state = SSL_FIRST_CONNECT;
            ssl = SSL_new(ssl_ctx_);
			ssl_filter = std::make_shared<SSLStreamBufferFilter>(stream_buffer, ssl);
			stream_buffer->AddStreamBufferFilter(ssl_filter);    
            break;        
        case SSL_FIRST_CONNECT:        
            cout << elapsed << "connected SSL - upgrading to SSL over SSL" << endl; 
            state = SSL_SECOND_CONNECT;
            ssl = SSL_new(ssl_ctx_);
            ssl_filter = std::make_shared<SSLStreamBufferFilter>(stream_buffer, ssl);
            stream_buffer->AddStreamBufferFilter(ssl_filter);    
            break;           
        case SSL_SECOND_CONNECT:
            cout << elapsed << "connected SSL over SSL" << endl;     
            break;
        default:
            throw "unexpected case";
        }
    }

	void HandleEOF(std::shared_ptr<StreamBuffer> stream_buffer) override {
        cout << elapsed << "connection closed" << endl;
        streams_buffers_.erase(stream_buffer);
    }
private:
    shared_ptr<EventLoop> event_loop_;
    SSL_CTX *ssl_ctx_;
    unordered_map<std::shared_ptr<StreamBuffer>, int> streams_buffers_;
};

int main() 
{
    cout << elapsed << "ping server started" << endl;
    auto event_loop = EventLoop::Create();
    auto ping_server = make_shared<PingServer>(event_loop);
    auto stream_listener = StreamListener::Create(ping_server, "127.0.0.1", 12345);
    event_loop->Attach(stream_listener);

    promise<void>().get_future().wait();
}