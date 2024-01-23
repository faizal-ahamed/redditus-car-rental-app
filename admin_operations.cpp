#include <mysql_driver.h>
#include <mysql_error.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <string>
#include "admin_operations.h"
#include <limits>


AdminOperations::AdminOperations(sql::Connection* connection) : conn(connection) {}


void AdminOperations::showAvailableCars() {
    try {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = conn->prepareStatement("SELECT id, make, model, year FROM cars");
        res = pstmt->executeQuery();

        std::cout << "\n                  AVAILABLE CARS                         \n";
        std::cout << "ID\tMake\tModel\tYear" << std::endl;
        while (res->next()) {
            std::cout << res->getInt("id") << "\t"
                      << res->getString("make") << "\t"
                      << res->getString("model") << "\t"
                      << res->getInt("year") << std::endl;
        }

        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cout << "Error retrieving available cars: " << e.what() << std::endl;
    }
}



void AdminOperations::addCar(const std::string& make, const std::string& model, int year) {
    try {
        sql::PreparedStatement* pstmt;
        std::string tableName = "cars";
        pstmt = conn->prepareStatement("INSERT INTO " + tableName + " (make, model, year) VALUES (?, ?, ?)");
        pstmt->setString(1, make);
        pstmt->setString(2, model);
        pstmt->setInt(3, year);
        pstmt->execute();
        std::cout << "\t CAR ADDED SUCCESSFULLY!" << std::endl;
        delete pstmt;
    }
    catch (sql::SQLException &e) {
        std::cout << "\t ERROR ADDING CAR: " << e.what() << std::endl;
    }
}

void AdminOperations::removeCar(int carId) {
    try {
        sql::PreparedStatement* pstmt;
        std::string tableName = "cars";
        pstmt = conn->prepareStatement("DELETE FROM " + tableName + " WHERE id = ?");
        pstmt->setInt(1, carId);
        pstmt->execute();
        std::cout << "\t CAR REMOVED SUCCESSFULLY!" << std::endl;
        delete pstmt;
    }
    catch (sql::SQLException &e) {
        std::cout << "Error removing car: " << e.what() << std::endl;
    }
}

void AdminOperations::updateCar(int carId, const std::string& make, const std::string& model, int year) {
    try {
        sql::PreparedStatement* pstmt;
        pstmt = conn->prepareStatement("UPDATE cars SET make = ?, model = ?, year = ? WHERE id = ?");
        pstmt->setString(1, make);
        pstmt->setString(2, model);
        pstmt->setInt(3, year);
        pstmt->setInt(4, carId);
        pstmt->execute();
        std::cout << "\t CAR UPDATED SUCCESSFULLY!" << std::endl;
        delete pstmt;
    }
    catch (sql::SQLException &e) {
        std::cout << "Error updating car: " << e.what() << std::endl;
    }
}

void AdminOperations::viewAllTransactions() {
    try {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;
        pstmt = conn->prepareStatement("SELECT * FROM transactions");
        res = pstmt->executeQuery();
        std::cout << "Transaction Details:" << std::endl;
        std::cout << "ID\tCar ID\tCustomer Name" << std::endl;
        while (res->next()) {
            std::cout << res->getInt("id") << "\t"
                 << res->getInt("car_id") << "\t"
                 << res->getString("customer_name") << std::endl;
        }
        delete res;
        delete pstmt;
    }
    catch (sql::SQLException &e) {
        std::cout << "Error retrieving transaction details: " << e.what() << std::endl;
    }
}



void AdminOperations::viewPotentialCars() {
    try {
        sql::PreparedStatement* pstmt_select;
        sql::ResultSet* res;

        pstmt_select = conn->prepareStatement("SELECT * FROM potentialcars");
        res = pstmt_select->executeQuery();

        std::cout << "POTENTIAL CARS:\n";
        std::cout << "----------------\n";

        while (res->next()) {
            int carId = res->getInt("id");
            std::string make = res->getString("make");
            std::string model = res->getString("model");
            int year = res->getInt("year");

            std::cout << "Car ID: " << carId << "\nMake: " << make << "\nModel: " << model << "\nYear: " << year << std::endl;

            std::cout << "Options:\n";
            std::cout << "1. Approve\n";
            std::cout << "2. Deny\n";
            std::cout << "3. Exit\n";
            int option;
            std::cout << "Choose option: ";
            
            while (!(std::cin >> option)) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Invalid input. Please enter a valid integer: ";
}

            if (option == 1) {
                sql::PreparedStatement* pstmt_approve;

                pstmt_approve = conn->prepareStatement("INSERT INTO cars (make, model, year, booked) VALUES (?, ?, ?, FALSE)");
                pstmt_approve->setString(1, make);
                pstmt_approve->setString(2, model);
                pstmt_approve->setInt(3, year);
                pstmt_approve->execute();

                pstmt_approve = conn->prepareStatement("DELETE FROM potentialcars WHERE id = ?");
                pstmt_approve->setInt(1, carId);
                pstmt_approve->execute();

                std::cout << "Car approved and added to inventory.\n";
            } else if (option == 2) {

                sql::PreparedStatement* pstmt_deny;

                pstmt_deny = conn->prepareStatement("DELETE FROM potentialcars WHERE id = ?");
                pstmt_deny->setInt(1, carId);
                pstmt_deny->execute();

                std::cout << "Car denied and removed from potential cars.\n";
            } 
            else if(option == 3)
            {
                break;
            }
            
            else {
                std::cout << "Invalid option. Skipping to the next potential car.\n";
            }
        }

        delete pstmt_select;
        delete res;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
    }
}