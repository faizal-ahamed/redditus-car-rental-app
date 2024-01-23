#ifndef ADMIN_OPERATIONS_H
#define ADMIN_OPERATIONS_H

#include <string>
#include <mysql_connection.h>

class AdminOperations {
public:
    AdminOperations(sql::Connection* connection);
    void showAvailableCars();
    void addCar(const std::string& make, const std::string& model, int year);
    void removeCar(int carId);
    void updateCar(int carId, const std::string& make, const std::string& model, int year);
    void viewAllTransactions();
    void viewPotentialCars();

private:
    sql::Connection* conn;
};

#endif 
