#include "like_session.h"
#include "like_session_delegate.h"
#include <json/json.h>
#include <boost/bind.hpp>


LikeSession::LikeSession(boost::asio::io_service& io_service)
    : io_service_(io_service), socket_(io_service), delegate_(0) {
    printf("[INFO] session ready (constructed).\n");
}

LikeSession::~LikeSession(void) {
    printf("[INFO] session closed (distructed).\n");
}

void LikeSession::BindDelegate(LikeSessionDelegate* delegate) {
    delegate_ = delegate;
}

void LikeSession::Start(void) {
    boost::asio::async_read(socket_
        , boost::asio::buffer(read_msg_.data(), chat_message::header_length)
        , boost::bind(&LikeSession::handle_read_header, shared_from_this(), boost::asio::placeholders::error));
}

void LikeSession::Close(void) {
    socket_.close();
}

Tcp::socket& LikeSession::socket(void) {
    return socket_;
}

void LikeSession::deliver(const chat_message& msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        boost::asio::async_write(socket_
            , boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length())
            , boost::bind(&LikeSession::handle_write, shared_from_this(), boost::asio::placeholders::error));
    }
}

void LikeSession::handle_read_header(const boost::system::error_code& error) {
    if (!error && read_msg_.decode_header()) {
        boost::asio::async_read(socket_
            , boost::asio::buffer(read_msg_.body(), read_msg_.body_length())
            , boost::bind(&LikeSession::handle_read_body, shared_from_this(), boost::asio::placeholders::error));
    } else {
        //io_service_.post(boost::bind(&LikeSessionDelegate::OnDisconnected, delegate_, shared_from_this()));
    }
}

void LikeSession::handle_read_body(const boost::system::error_code& error) {
    if (!error) {
        // room_.deliver(read_msg_);
        boost::asio::async_read(socket_
            , boost::asio::buffer(read_msg_.data(), chat_message::header_length)
            , boost::bind(&LikeSession::handle_read_header, shared_from_this(), boost::asio::placeholders::error));





        // TODO(jh81.kim): 

        const char* begin = read_msg_.body();
        const char* end = begin + read_msg_.body_length();
        const std::string json(begin, end);

        Json::Value root(Json::objectValue);
        Json::Reader reader;
        if (!reader.parse(json, root, false)) {
            // TODO(jh81.kim): something wrong
            return;
        }

        const std::string query = root["query"].asString();
        if (query == "open") {
            const Json::Value& user = root["user"];
            if (user.isString()) {
                const std::string param = user.asString();
                io_service_.post(boost::bind(&LikeSessionDelegate::OnOpen, delegate_, shared_from_this(), param));
            }
        } else if (query == "close") {
            const Json::Value& user = root["user"];
            if (user.isString()) {
                const std::string param = user.asString();
                io_service_.post(boost::bind(&LikeSessionDelegate::OnClose, delegate_, shared_from_this(), param));
            }
        } else if (query == "join") {
            const Json::Value& user = root["user"];
            const Json::Value& target = root["target"];
            if (user.isString() && target.isString()) {
                const std::string param1 = user.asString();
                const std::string param2 = target.asString();
                io_service_.post(boost::bind(&LikeSessionDelegate::OnJoin, delegate_, shared_from_this(), param1, param2));
            }
        } else if (query == "leave") {
            // TODO(jh81.kim): 
        } else if (query == "like") {
            // TODO(jh81.kim): 
        } else {
            // nothing
        }





    } else {
        //io_service_.post(boost::bind(&LikeSessionDelegate::OnDisconnected, delegate_, shared_from_this()));
    }
}

void LikeSession::handle_write(const boost::system::error_code& error) {
    if (!error) {
        write_msgs_.pop_front();
        if (!write_msgs_.empty()) {
            boost::asio::async_write(socket_
                , boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length())
                , boost::bind(&LikeSession::handle_write, shared_from_this(), boost::asio::placeholders::error));
        }
    } else {
        //io_service_.post(boost::bind(&LikeSessionDelegate::OnDisconnected, delegate_, shared_from_this()));
    }
}