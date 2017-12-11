//
// Created by kchen on 12/10/17.
//

#ifndef SPLUNK_HEC_CLIENT_CPP_TIMEOUT_EXCEPTION_H
#define SPLUNK_HEC_CLIENT_CPP_TIMEOUT_EXCEPTION_H

#include <exception>
#include <string>

namespace concurrentcpp {

    class TimeoutException: public std::exception {
    public:
        explicit TimeoutException(const std::string& msg): msg_(msg) {
        }

        const char* what() const noexcept override {
            return msg_.c_str();
        }

    private:
        std::string msg_;
    };

} // namespace concurrentcpp

#endif //SPLUNK_HEC_CLIENT_CPP_TIMEOUT_EXCEPTION_H
