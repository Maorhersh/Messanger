/*
In order to make the progam work you need to run in in Visual Studio 2022, press Project in upper menu
Then choose Mange NuGet Packages install the following :
boost
boost_date_time-vc143
boost_filesystem-vc143
boost_system-vc143
cryptopp
*/


#include "Menu.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <boost/algorithm/string/trim.hpp>

//In the constcructor we initialize the menu options and their corresponding functions.
Menu::Menu() {
    menuOptionsFunctions = {
        { CMenuOption::EOption::REGISTER,        [this]() { registerUser(); }},
        { CMenuOption::EOption::REQ_CLIENT_LIST,   [this]() { showClientList(); }},
        { CMenuOption::EOption::REQ_PUBLIC_KEY,    [this]() { requestPublicKey(); }},
        { CMenuOption::EOption::REQ_PENDING_MSG,   [this]() { showPendingMessages(); }},
        { CMenuOption::EOption::SEND_MSG,          [this]() { sendMessage(); }},
        { CMenuOption::EOption::REQ_SYM_KEY,       [this]() { requestSymmetricKey(); }},
        { CMenuOption::EOption::SEND_SYM_KEY,      [this]() { sendSymmetricKey(); }},
        { CMenuOption::EOption::SEND_FILE,         [this]() { sendFile(); }},
        { CMenuOption::EOption::EXIT,              [this]() { exitMessageU(); }}
    };
}


//This function Stops the client execution with an error message if there is an error.



//initializing the the client menu and initialize information .
void Menu::initialize() {
    if (!logicController.parseServeInfo()) {
        std::cout << "failed to read server info from file" << std::endl;
        exit(1);
    }
    isRegistered = logicController.parseClientInfo();
}

//This function displays the client menu with the welcoming message and the menu options
void Menu::display() const {
    if (isRegistered && !logicController.getSelfUsername().empty())
        std::cout << "Hello " << logicController.getSelfUsername() << ", ";
    std::cout << "MessageU client at your service." << std::endl << std::endl;
    for (const auto& opt : optionsResponse)
        std::cout << opt << std::endl;
}


std::string Menu::readInput(const std::string& description) const {
    std::string input;
    if (!description.empty()) {
        std::cout << description << std::endl;
    }

    while (true) {
        std::getline(std::cin, input);

        // If Ctrl+Z then clear the screen and continue
        if (std::cin.eof()) {

            system("cls");

            continue;
        }

        // Trim whitespace from both ends of the string
        boost::algorithm::trim(input);

 
        bool invalid = std::any_of(input.begin(), input.end(), [](unsigned char c) {
            // Ignore standard whitespace
            if (std::isspace(c)) {
                return false;
            }
            if (c > 127) {//what is over this value in ascii code is not allowed
                return true;
            }
            return false;
            });
        // If an invalid character exists, show an error message and retry
        if (invalid) {
            std::cout << "Please use only English chars, digits and whitespace." << std::endl;
            continue;
        }

        // If the trimmed input is empty - ignore it 
        if (input.empty()) {
            continue;
        }
        break;
    }

    return input;
}




//this function gets the user input annd checks if it is a valid option
bool Menu::getMenuOption(CMenuOption& menuOption) const {
    const std::string input = readInput();
    for (const auto& option : optionsResponse) {
        if (input == std::to_string(static_cast<uint32_t>(option.getValue()))) {
            menuOption = option;
            return true;
        }
    }
    return false;
}

//This function is for respones according to the user input
void Menu::handleClientChoice() {
    CMenuOption menuOption;
    while (!getMenuOption(menuOption)) {
        std::cout << "Invalid input. Please try again.." << std::endl;
    }
    if (!isRegistered && menuOption.requireRegistration()) {
        std::cout << "You must register first!" << std::endl;
        return;
    }
    auto handler = menuOptionsFunctions.find(menuOption.getValue());
    if (handler != menuOptionsFunctions.end()) {
       
        handler->second();
    }
    else {
        std::cout << "Invalid menu option." << std::endl;
    }
}



//this function registers the client and if hes registered already it tells him that.
void Menu::registerUser() {
    if (isRegistered) {
        std::cout << logicController.getSelfUsername()
            << ", you have already registered!" << std::endl;
        return;
    }
    const std::string username = readInput("Please type your username..");
    isRegistered = logicController.registerUser(username);
    if (isRegistered) {
        std::cout << "Successfully registered on server." << std::endl;
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}

//this function shows the client list
void Menu::showClientList() {
    if (logicController.requestClientsList()) {
        std::vector<std::string> usernames = logicController.getUsernames();
        if (usernames.empty()) {
            std::cout << "No useres in the server" << std::endl;
            return;
        }
        std::cout << "Registered users:" << std::endl;
        for (const auto& username : usernames) {
            std::cout << username << std::endl;
        }
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}

//this function requests the public key
void Menu::requestPublicKey() {
    const std::string username = readInput(USERNAME_OPENING);
    if (logicController.requestClientPublicKey(username)) {
        std::cout << "Public key has been returned from the server successfully." << std::endl;
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}

// this function shows the pending messages according to the task template
void Menu::showPendingMessages() {

    std::vector<MainLogic::Message> messages;
    if (logicController.requestPendingMessages(messages)) {
        std::cout << std::endl;
        for (const auto& msg : messages) {
            std::cout << "From: " << msg.username << std::endl;
            std::cout << "Content:" << std::endl;
            std::cout << msg.content << std::endl;
            std::cout << std::endl;
        }
        const std::string lastErr = logicController.getCurrentError();
        if (!lastErr.empty()) {
            std::cout << std::endl << "MESSAGES ERROR LOG: " << std::endl << lastErr;
        }
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}


//this function handles with sending a message to other user
void Menu::sendMessage() {
    const std::string username = readInput(USERNAME_OPENING + " to send message to..");
    const std::string message = readInput("Enter message: ");
    if (logicController.sendMessage(username, MSG_SEND_TEXT, message)) {
        std::cout << "message has been sent to the server sucssefully" << std::endl;
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}

//this function handles request for a symmetric key
void Menu::requestSymmetricKey() {
    const std::string username = readInput(USERNAME_OPENING + " to request symmetric key from..");
    if (logicController.sendMessage(username, MSG_SYMMETRIC_KEY_REQUEST)) {
        std::cout << "A request for a Symmetric key been sent sucssefully to the server" << std::endl;
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}

//this function handles with seding a symmetric key
void Menu::sendSymmetricKey() {
    const std::string username = readInput(USERNAME_OPENING + " to send symmetric key to..");
    if (logicController.sendMessage(username, MSG_SYMMETRIC_KEY_SEND)) {
        std::cout << "a request for sending your private Symmetric key to the server passed sucssefully" << std::endl;
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}

//this fucntio nhandles with sending a file
void Menu::sendFile() {
    const std::string username = readInput(USERNAME_OPENING + " to send file to..");
    const std::string message = readInput("Enter file name with extention (e.g. : file.txt): ");
    if (logicController.sendMessage(username, MSG_SEND_FILE, message)) {
        std::cout << "a request for sending file sucssefully issued" << std::endl;
    }
    else {
        std::cout << logicController.getCurrentError() << std::endl;
    }
}

//this function show the message we get when we exit the program
void Menu::exitMessageU() {
    std::cout << "You've exited MessageU, bye!" << std::endl;
    exit(1);
}



//start the program and loop through the menu function in otder to get the functionality of the menu
int main(int argc, char* argv[])
{
    Menu menu;
    menu.initialize();

    while (true)
    {
        menu.display();
        menu.handleClientChoice();
        system("pause");
        system("cls");
    }

    return 0;
}
