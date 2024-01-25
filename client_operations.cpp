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
                      << (isBooked ? (dob.empty() ? "NIL" : dob) : "NIL") << "\t"
                      << (isBooked ? (eob.empty() ? "NIL" : eob) : "NIL") << std::endl;
        }

        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cout << "Error retrieving cars details: " << e.what() << std::endl;
    }
}



void ClientOperations::bookCar( const std::string& customerName) {
    try {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&currentTime);
        std::stringstream currentDateStream;
        currentDateStream << std::put_time(localTime, "%Y-%m-%d");
        std::string currentDate = currentDateStream.str();

        

        while (true) {
    std::cout << "\t ENTER CAR ID TO BE BOOKED : ";
    while (!(std::cin >> carId)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a valid number: ";
    }

    sql::PreparedStatement* checkCarStmt = conn->prepareStatement("SELECT id FROM cars WHERE id = ?");
    checkCarStmt->setInt(1, carId);
    sql::ResultSet* checkCarResult = checkCarStmt->executeQuery();

    if (checkCarResult->next()) {
        break; 
    } else {
        std::cout << "\t CAR WITH ID " << carId << " DOES NOT EXIST." << std::endl;
    }
        

        delete checkCarResult;
        delete checkCarStmt;
        }

        while (true) {
        std::cout << "\t ENTER START DATE OF BOOKING (YYYY-MM-DD) : ";
        std::cin >> dob;
        if (dob.length() == 10) {
        break;  
            } else {
        std::cout << "\t Invalid input. Please enter a date in the format YYYY-MM-DD.\n";
            }
        }

        while (true) {
        std::cout << "\t ENTER END DATE OF BOOKING   (YYYY-MM-DD) : ";
        std::cin >> eob;
        if (eob.length() == 10) {
            break;  
            } else {
        std::cout << "\t Invalid input. Please enter a date in the format YYYY-MM-DD.\n";
        }
    }

        if (dob < currentDate || eob < currentDate) {
            std::cout << "\t Date of Booking and End of Booking should be equal to or greater than the current date." << std::endl;
            return;
        }

        if (eob <= dob) {
            std::cout << "\t End of Booking should be greater than Date of Booking." << std::endl;
            return;
        }

        conn->setAutoCommit(false);

        

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


int ClientOperations::NotReturned(const std::string& customerName) {
    try {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&currentTime);

        std::stringstream currentDateStream;
        currentDateStream << std::put_time(localTime, "%Y-%m-%d");
        std::string currentDate = currentDateStream.str();

        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = conn->prepareStatement("SELECT DISTINCT c.id, c.make, c.model, c.year FROM transactions t INNER JOIN cars c ON t.car_id = c.id WHERE t.customer_name = ? AND c.booked=TRUE AND t.eob <= ? AND returnedstatus=0");
        pstmt->setString(1, customerName);
        pstmt->setString(2, currentDate);
        res = pstmt->executeQuery();

        std::cout << "Details of booked cars for customer '" << customerName << "' not returned:\n";
        std::cout << "Id\t" << "Make\t" << "Model\t" << "Year\t\n";

        bool foundRecords = false;

        while (res->next()) {
            foundRecords = true;
            std::cout << res->getInt("id") << "\t" << res->getString("make") << "\t" << res->getString("model") << "\t" << res->getInt("year") << "\n";
        }

        if (!foundRecords) {
            std::cout << "No not returned cars found for customer '" << customerName << "'.\n";
        }

        std::cout << "\n";
        delete res;
        delete pstmt;
        int r=0;
    if(foundRecords)
    r=1;
    return r;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << "\n";
    }
    return 0;
    
}



void ClientOperations::returnCar(const std::string& customerName) {
     try {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&currentTime);
        std::stringstream currentDateStream;
        currentDateStream << std::put_time(localTime, "%Y-%m-%d");
        std::string currentDate = currentDateStream.str();

        int carIdToBeReturned;
        std::cout<<"\t ENTER CAR ID TO BE RETURNED : ";
        while (!(std::cin >> carIdToBeReturned)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a valid number: ";
        }



        sql::PreparedStatement* checkCarBookingStmt = conn->prepareStatement("SELECT id FROM transactions WHERE car_id = ? AND customer_name = ? AND eob <= ?");
        checkCarBookingStmt->setInt(1, carIdToBeReturned);
        checkCarBookingStmt->setString(2, customerName);
        checkCarBookingStmt->setString(3, currentDate);
        sql::ResultSet* checkCarBookingResult = checkCarBookingStmt->executeQuery();


        if (!checkCarBookingResult->next()) {
            std::cout << "You have not booked car with ID " << carIdToBeReturned << ". Cannot return the car.\n";
            delete checkCarBookingResult;
            delete checkCarBookingStmt;
            return;
        }

        delete checkCarBookingResult;
        delete checkCarBookingStmt;


        sql::PreparedStatement* changeTransactionsStmt = conn->prepareStatement("UPDATE transactions SET returnedstatus = 1 WHERE car_id = ? AND eob <= ? AND customer_name=?");
        changeTransactionsStmt->setInt(1, carIdToBeReturned);
        changeTransactionsStmt->setString(2, currentDate);
        changeTransactionsStmt->setString(3, customerName);
        sql::ResultSet* changeTransactionsResult = changeTransactionsStmt->executeQuery();
        
        
        
        sql::PreparedStatement* checkTransactionsStmt = conn->prepareStatement("SELECT id FROM transactions WHERE car_id = ? AND returnedstatus = 0");
        checkTransactionsStmt->setInt(1, carIdToBeReturned);
        sql::ResultSet* checkTransactionsResult = checkTransactionsStmt->executeQuery();

        if (!checkTransactionsResult->next()) {
            sql::PreparedStatement* updateCarStmt = conn->prepareStatement("UPDATE cars SET booked = FALSE WHERE id = ?");
            updateCarStmt->setInt(1, carIdToBeReturned);
            updateCarStmt->execute();
            delete updateCarStmt;


        } 

          std::cout << "Car with ID " << carIdToBeReturned<< " returned successfully.\n";

        delete checkTransactionsResult;
        delete checkTransactionsStmt;
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

        delete pstmt;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
    }
}




void ClientOperations::displayClientTransactions(const std::string& clientUsername) {
    try {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = conn->prepareStatement("SELECT c.id, c.make, c.model, c.year,t.dob,t.eob FROM transactions t INNER JOIN cars c ON t.car_id = c.id WHERE t.customer_name = ?");
        pstmt->setString(1, clientUsername);
        res = pstmt->executeQuery();

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

        delete pstmt;
        delete res;
    } catch (sql::SQLException &e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
    }
}
