# libael_openssl
An OpenSSL filter for stream buffers in the C++ [libael](https://github.com/TomerHeber/libael) library.
## Install
```shell
git clone https://github.com/TomerHeber/libael_openssl
cd libael && mkdir build && cd build
cmake ..
```
##### Compile
```shell
cmake --build .
```
##### Compile & Install
```shell
cmake --build . --target install
```
## Platforms
* Linux
> Additional platforms may be added in the future (please open feature requests).
## Features
* OpenSSL support for the [libael](https://github.com/TomerHeber/libael) stream buffers.
> Additional features may be added in the future (please open feature requests).
## Usage Samples
Sample files are available in the [samples](samples/) directory
#### Create an SSL over SSL "pingpong" Server and Client
The filter may be added to the stream buffer during the "HandleConnected" callback method (view code below).
In this case "HandleConnected" will be called 3 times:
* 1st on TCP Connected.
* 2nd on SSL Connected.
* 3rd on SSL over SSL Connected.

Once the tunnel has been established the client will send a "ping" string. The server will reply with a "pong". When the client receives the "pong" it will close the connection (this will also do proper SSL_shutdown).
###### Code - Server
```c++
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
```
###### Code - Client
```c++
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
```
###### Output - Server
```shell
[thread #139661554677568][   0ms] ping server started
[thread #139661522036480][13323ms] new connection
[thread #139661522036480][13323ms] connected TCP - upgrading to SSL
[thread #139661522036480][13335ms] connected SSL - upgrading to SSL over SSL
[thread #139661522036480][13338ms] connected SSL over SSL
[thread #139661522036480][13339ms] connection closed
```
###### Output - Client
```shell
[thread #139822899824448][   0ms] ping client started
[thread #139822867183360][   3ms] connected TCP - upgrading to SSL
[thread #139822867183360][  14ms] connected SSL - upgrading to SSL over SSL
[thread #139822867183360][  17ms] connected SSL over SSL, sending ping
[thread #139822867183360][  18ms] pong received, closing connection
[thread #139822867183360][  19ms] connection closed
[thread #139822899824448][  20ms] ping client finished
```
## Documentation
At the moment there is no documentation available. For any questions or concerns please open an issue. Documentation will be added in time.
