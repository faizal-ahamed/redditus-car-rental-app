#ifndef CLIENT_OPERATIONS_H
#define CLIENT_OPERATIONS_H

#include <string>
#include <mysql_connection.h>

class ClientOperations {
public:
    ClientOperations(sql::Connection* connection);
    void showAvailableCars();
    void bookCar(int carId, const std::string& customerName, const std::string& dob, const std::string& eob);
    void NotReturned(const std::string& customerName);
    void returnCar(int carId);
    void postCarForRent( const std::string& make, const std::string& model,int year);
    void displayClientTransactions( const std::string& clientUsername);
private:
    sql::Connection* conn;
};

#endif 

