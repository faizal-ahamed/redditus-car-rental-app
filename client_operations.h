#ifndef CLIENT_OPERATIONS_H
#define CLIENT_OPERATIONS_H

#include <string>
#include <mysql_connection.h>

class ClientOperations {
public:
    ClientOperations(sql::Connection* connection);
    void showAvailableCars();
    // void bookCar(int carId, const std::string& customerName, const std::string& dob, const std::string& eob);
    void bookCar(const std::string& customerName);
    int NotReturned(const std::string& customerName);
    void returnCar(const std::string& customerName);
    void postCarForRent( const std::string& make, const std::string& model,int year);
    void displayClientTransactions( const std::string& clientUsername);
private:
    sql::Connection* conn;
    int carId;
    std::string dob,eob;

};

#endif 

