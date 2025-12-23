#pragma once
#include "MainLogic.h"
#include <string>
#include <iomanip>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <vector>
#include <boost/algorithm/string/trim.hpp>

// This class is responsible for the client menu.
class Menu
{
public:
    const std::string USERNAME_OPENING = "Please type a username";
    Menu();

    void initialize();
    void display() const;
    void handleClientChoice();


    friend std::ostream& operator<<(std::ostream& os, const Menu* menu) {
        if (menu != nullptr)
            os << "MessageU Client Menu";
        return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const Menu& menu) {
        return operator<<(os, &menu);
    }

    Menu(const Menu&) = delete;
    Menu(Menu&&) noexcept = delete;
    Menu& operator=(const Menu&) = delete;
    Menu& operator=(Menu&&) noexcept = delete;

private:

    class CMenuOption
    {
    public:
        enum class EOption
        {
            REGISTER = 110,
            REQ_CLIENT_LIST = 120,
            REQ_PUBLIC_KEY = 130,
            REQ_PENDING_MSG = 140,
            SEND_MSG = 150,
            REQ_SYM_KEY = 151,
            SEND_SYM_KEY = 152,
            SEND_FILE = 153,
            EXIT = 0
        };

    private:
        EOption _value;
        bool _registration;
        std::string _description;
        std::string _success;

    public:
        CMenuOption() : _value(EOption::EXIT), _registration(false) {}
        CMenuOption(EOption val, bool reg, std::string desc, std::string success)
            : _value(val), _registration(reg), _description(std::move(desc)), _success(std::move(success))
        {
        }

        friend std::ostream& operator<<(std::ostream& os, const CMenuOption& opt) {
            os << std::setw(2) << static_cast<uint32_t>(opt._value)
                << ") " << opt._description;
            return os;
        }

        EOption getValue() const { return _value; }
        bool requireRegistration() const { return _registration; }
        std::string getDescription() const { return _description; }
        std::string getSuccessString() const { return _success; }
    };

    std::string readInput(const std::string& description = "") const;
    bool getMenuOption(CMenuOption& menuOption) const;

    void registerUser();
    void showClientList();
    void requestPublicKey();
    void showPendingMessages();
    void sendMessage();
    void requestSymmetricKey();
    void sendSymmetricKey();
    void sendFile();
    void exitMessageU();

    MainLogic logicController;
    bool isRegistered = false;

    const std::vector<CMenuOption> optionsResponse{
        { CMenuOption::EOption::REGISTER,        false, "Register",                         "Successfully registered on server." },
        { CMenuOption::EOption::REQ_CLIENT_LIST,   true,  "Request client list",              "" },
        { CMenuOption::EOption::REQ_PUBLIC_KEY,    true,  "Request public key",               "Public key retrieved." },
        { CMenuOption::EOption::REQ_PENDING_MSG,   true,  "Request pending messages",         "" },
        { CMenuOption::EOption::SEND_MSG,          true,  "Send text message",                "Message sent." },
        { CMenuOption::EOption::REQ_SYM_KEY,       true,  "Request symmetric key",            "Symmetric key requested." },
        { CMenuOption::EOption::SEND_SYM_KEY,      true,  "Send symmetric key",               "Symmetric key sent." },
        { CMenuOption::EOption::SEND_FILE,         true,  "Send file",                        "File sent." },
        { CMenuOption::EOption::EXIT,              false, "Exit client",                      "" }
    };

    std::unordered_map<CMenuOption::EOption, std::function<void()>> menuOptionsFunctions;
};
