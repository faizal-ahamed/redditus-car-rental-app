#include "client_operations.h"
#include <mysql_driver.h>
#include <mysql_error.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <chrono>
#include <ctime>
#include <iomanip>


ClientOperations::ClientOperations(sql::Connection* connection) : conn(connection) {}

void ClientOperations::showAvailableCars() {
    try {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = conn->prepareStatement("SELECT cars.id, cars.make, cars.model, cars.year, cars.booked, transactions.dob, transactions.eob FROM cars LEFT JOIN transactions ON cars.id = transactions.car_id");
        res = pstmt->executeQuery();

        std::cout << "\n                  CARS DETAILS                         \n";
        std::cout << "ID\tMake\tModel\tYear\tBooked\tBooked From\tBooked To" << std::endl;
        while (res->next()) {
            int carId = res->getInt("id");
            std::string make = res->getString("make");
            std::string model = res->getString("model");
            int year = res->getInt("year");
            bool isBooked = res->getBoolean("booked");
            std::string dob = res->getString("dob");
            std::string eob = res->getString("eob");

            std::cout << carId << "\t"
                      << make << "\t"
                      << model << "\t"
                      << year << "\t"
                      << (isBooked ? "TRUE" : "FALSE") << "\t"
                      << (isBooked ? (dob.empty() ? "N/A" : dob) : "N/A") << "\t"
                      << (isBooked ? (eob.empty() ? "N/A" : eob) : "N/A") << std::endl;
        }

        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cout << "Error retrieving cars details: " << e.what() << std::endl;
    }
}



void ClientOperations::bookCar(int carId, const std::string& customerName, const std::string& dob, const std::string& eob) {
    try {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&currentTime);
        std::stringstream currentDateStream;
        currentDateStream << std::put_time(localTime, "%Y-%m-%d");
        std::string currentDate = currentDateStream.str();

        if (dob < currentDate || eob < currentDate) {
            std::cout << "\t Date of Booking and End of Booking should be equal to or greater than the current date." << std::endl;
            return;
        }

        if (eob <= dob) {
            std::cout << "\t End of Booking should be greater than Date of Booking." << std::endl;
            return;
        }

        conn->setAutoCommit(false);

        sql::PreparedStatement* checkCarStmt = conn->prepareStatement("SELECT id FROM cars WHERE id = ?");
        checkCarStmt->setInt(1, carId);
        sql::ResultSet* checkCarResult = checkCarStmt->executeQuery();

        if (!checkCarResult->next()) {
            std::cout << "\t CAR WITH ID " << carId << " DOES NOT EXIST." << std::endl;
            delete checkCarResult;
            delete checkCarStmt;
            conn->rollback();
            return;
        }

        delete checkCarResult;
        delete checkCarStmt;

        sql::PreparedStatement* checkStmt = conn->prepareStatement("SELECT id FROM transactions WHERE car_id = ? AND ((dob <= ? AND eob >= ?) OR (dob <= ? AND eob >= ?) OR (dob >= ? AND eob <= ?))");
        checkStmt->setInt(1, carId);
        checkStmt->setString(2, dob);
        checkStmt->setString(3, dob);
        checkStmt->setString(4, eob);
        checkStmt->setString(5, eob);
        checkStmt->setString(6, dob);
        checkStmt->setString(7, eob);
        sql::ResultSet* checkResult = checkStmt->executeQuery();

        if (checkResult->next()) {
            std::cout << "\t CAR WITH ID " << carId << " IS ALREADY BOOKED WITHIN THE SPECIFIED DATE RANGE." << std::endl;
            delete checkResult;
            delete checkStmt;
            conn->rollback();
            return;
        }

        delete checkResult;
        delete checkStmt;

        sql::PreparedStatement* pstmt = conn->prepareStatement("INSERT INTO transactions (car_id, customer_name, dob, eob) VALUES (?, ?, ?, ?)");
        pstmt->setInt(1, carId);
        pstmt->setString(2, customerName);
        pstmt->setString(3, dob);
        pstmt->setString(4, eob);
        pstmt->execute();
        delete pstmt;

        pstmt = conn->prepareStatement("UPDATE cars SET booked = TRUE WHERE id = ?");
        pstmt->setInt(1, carId);
        pstmt->execute();
        delete pstmt;

        int fakeid = -1;
        sql::ResultSet* res;
        sql::PreparedStatement* pstmt1 = conn->prepareStatement("SELECT id FROM transactions ORDER BY id DESC LIMIT 1");
        res = pstmt1->executeQuery();
        if (res->next()) {
            fakeid = res->getInt("id");
        }
        delete res;
        delete pstmt1;

        if (fakeid != -1) {
            pstmt = conn->prepareStatement("SELECT DATEDIFF(eob, dob) AS date_difference FROM transactions WHERE id=?");
            pstmt->setInt(1, fakeid);
            res = pstmt->executeQuery();
            if (res->next()) {
                int dateDifference = res->getInt("date_difference");
                std::cout << "You have booked car with carID " << carId << " from " << dob << " to " << eob << " for " << dateDifference << " days" << std::endl;
            }
            delete res;
            delete pstmt;
        }

        conn->commit();
        std::cout << "Car booked successfully!" << std::endl;
    } catch (sql::SQLException& e) {
        std::cout << "Error booking car: " << e.what() << std::endl;
        conn->rollback();
    }
}


void ClientOperations::NotReturned(const std::string& customerName) {
    try {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&currentTime);

        std::stringstream currentDateStream;
        currentDateStream << std::put_time(localTime, "%Y-%m-%d");
        std::string currentDate = currentDateStream.str();

        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = conn->prepareStatement("SELECT DISTINCT c.id, c.make, c.model, c.year FROM transactions t INNER JOIN cars c ON t.car_id = c.id WHERE t.customer_name = ? AND c.booked=TRUE AND t.eob >= ?");
        pstmt->setString(1, customerName);
        pstmt->setString(2, currentDate);
        res = pstmt->executeQuery();

        std::cout << "Details of booked cars for customer '" << customerName << "' not returned:\n";
        std::cout << "Id\t" << "Make\t" << "Model\t" << "Year\t\n";

        while (res->next()) {
            std::cout << res->getInt("id") << "\t" << res->getString("make") << "\t" << res->getString("model") << "\t" << res->getInt("year") << "\n";
        }

        std::cout << "\n";
        delete res;
        delete pstmt;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << "\n";
    }
}


void ClientOperations::returnCar(int carId) {
    try {
        sql::PreparedStatement* pstmt;
        pstmt = conn->prepareStatement("UPDATE cars SET booked = FALSE WHERE id = ?");
        pstmt->setInt(1, carId);
        pstmt->execute();

        std::cout << "Car with ID " << carId << " returned successfully.\n";

        delete pstmt;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << "\n";
    }
}

void ClientOperations::postCarForRent(const std::string& make, const std::string& model,int year) {
    try {
        sql::PreparedStatement* pstmt;
        pstmt = conn->prepareStatement("INSERT INTO potentialcars (make, model, year, booked) VALUES (?, ?, ?, FALSE)");
        pstmt->setString(1, make);
        pstmt->setString(2, model);
        pstmt->setInt(3, year);
        pstmt->execute();

        std::cout << "Car posted for renting successfully." << std::endl;

        // Clean up
        delete pstmt;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
    }
}




void ClientOperations::displayClientTransactions(const std::string& clientUsername) {
    try {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        // Prepare SQL statement to retrieve transactions for the given client
        pstmt = conn->prepareStatement("SELECT c.id, c.make, c.model, c.year,t.dob,t.eob FROM transactions t INNER JOIN cars c ON t.car_id = c.id WHERE t.customer_name = ?");
        pstmt->setString(1, clientUsername);
        res = pstmt->executeQuery();

        // Display the transactions
        std::cout << "================================================================================\n";
        std::cout << "                     TRANSACTIONS FOR CLIENT: " << clientUsername << "                     \n";
        std::cout << "================================================================================\n";

        while (res->next()) {
            std::cout << "Car ID: " << res->getInt("id") << "\n";
            std::cout << "Make: " << res->getString("make") << "\n";
            std::cout << "Model: " << res->getString("model") << "\n";
            std::cout << "Year: " << res->getInt("year") << "\n";
            std::cout << "Dob: " << res->getString("dob") << "\n";
            std::cout << "Eob: " << res->getString("eob") << "\n";
            std::cout << "---------------------------------\n";
        }

        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
    }
}
