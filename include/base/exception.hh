#pragma once
#include <exception>
#include <string>

class aException : public std::exception {
public:
    explicit aException(int exceptNo , std::string const & message) : m_exceptNo(exceptNo),m_message(message) {}
    explicit aException(std::string const & message) : m_exceptNo(-1),m_message(message) {}
    ~aException(){}

    const char* what() const noexcept override {
        return m_message.c_str();
    }

    int number() const noexcept {
        return m_exceptNo;
    }

private:
    int m_exceptNo;
    std::string m_message;
};

using a_exception = aException;