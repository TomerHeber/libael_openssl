#include <iostream>
#include <chrono>
#include <memory>
#include <future>

#include <ael/event_loop.h>
#include <ael/stream_buffer.h>

#include <ael/openssl/ssl_stream_buffer_filter.h>

#include "elapsed.h"

using namespace std;
using namespace ael;

static Elapsed elapsed;

#define TCP_CONNECT 2
#define SSL_FIRST_CONNECT 1
#define SSL_SECOND_CONNECT 0

class PingClient : public StreamBufferHandler {
public:
    PingClient(shared_ptr<promise<void>> done_promise) : done_promise_(done_promise), state_(TCP_CONNECT) {
       ssl_ctx_ = SSL_CTX_new(SSLv23_client_method());
    }
    virtual ~PingClient() {
        SSL_CTX_free(ssl_ctx_);
    }

    void HandleData(std::shared_ptr<StreamBuffer> stream_buffer, const std::shared_ptr<const DataView> &data_view) override {
        string data;
        data_view->AppendToString(data);

        if (data.find("pong") != string::npos) {
            cout << elapsed << "pong received, closing connection" << endl;
            stream_buffer->Close();
        }
    }

	void HandleConnected(std::shared_ptr<StreamBuffer> stream_buffer) override { 
        SSL *ssl;
        shared_ptr<SSLStreamBufferFilter> ssl_filter;

        switch(state_) {
        case TCP_CONNECT:
            cout << elapsed << "connected TCP - upgrading to SSL" << endl;
            state_ = SSL_FIRST_CONNECT;
            ssl = SSL_new(ssl_ctx_);
			ssl_filter = std::make_shared<SSLStreamBufferFilter>(stream_buffer, ssl);
			stream_buffer->AddStreamBufferFilter(ssl_filter);    
            break;        
        case SSL_FIRST_CONNECT:        
            cout << elapsed << "connected SSL - upgrading to SSL over SSL" << endl; 
            state_ = SSL_SECOND_CONNECT;
            ssl = SSL_new(ssl_ctx_);
            ssl_filter = std::make_shared<SSLStreamBufferFilter>(stream_buffer, ssl);
            stream_buffer->AddStreamBufferFilter(ssl_filter);    
            break;           
        case SSL_SECOND_CONNECT:
            cout << elapsed << "connected SSL over SSL, sending ping" << endl;
            stream_buffer->Write(string("ping"));
            break;
        default:
            throw "unexpected case";
        }    
    }

	void HandleEOF(std::shared_ptr<StreamBuffer> stream_buffer) override {
        cout << elapsed << "connection closed" << endl;
        // Release the "main" thread.
        done_promise_->set_value();
    }
private:
    shared_ptr<promise<void>> done_promise_;
    SSL_CTX *ssl_ctx_;
    int state_;
};

int main() 
{
    cout << elapsed << "ping client started" << endl;
    
    auto event_loop = EventLoop::Create();
    auto done_promise = make_shared<promise<void>>();
    shared_future<void> done_future(done_promise->get_future());
    auto ping_client = make_shared<PingClient>(done_promise);
    auto stream_buffer = StreamBuffer::CreateForClient(ping_client, "127.0.0.1", 12345);
    event_loop->Attach(stream_buffer);

    done_future.wait();
    EventLoop::DestroyAll();

    cout << elapsed << "ping client finished" << endl;
}