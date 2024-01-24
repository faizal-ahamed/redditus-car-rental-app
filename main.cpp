#include <iostream>
#include "admin_operations.h"
#include "client_operations.h"
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <limits>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include  <bits/stdc++.h> 
#include <termios.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <sstream>
#include <openssl/evp.h>

using namespace std;




// Function to establish a connection to the MySQL database
sql::Connection* establishDBConnection()
{   
  sql::mysql::MySQL_Driver* driver;
  sql::Connection* con;
  driver = sql::mysql::get_mysql_driver_instance();
  con = driver->connect("tcp://127.0.0.1:3306", "root", "missingme");
  con->setSchema("mydatabase");
  return con;
}


void disableEcho() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~(ECHO | ECHONL); 
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void enableEcho() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= (ECHO | ECHONL); 
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}


string sha256(const string& input) {
    EVP_MD_CTX *ctx;
    const EVP_MD *type = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;

    ctx = EVP_MD_CTX_new();
    EVP_DigestInit(ctx, type);
    EVP_DigestUpdate(ctx, input.c_str(), input.length());
    EVP_DigestFinal(ctx, hash, &hashLen);

    EVP_MD_CTX_free(ctx);
    stringstream ss;
    for(unsigned int i = 0; i < hashLen; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }

    return ss.str();
}


bool checkUsernameExists(sql::Connection* con, const string& username) {
    try {
        string query = "SELECT COUNT(*) FROM clients WHERE username = ?";
        sql::PreparedStatement* pstmt = con->prepareStatement(query);
        pstmt->setString(1, username);
        sql::ResultSet* resultSet = pstmt->executeQuery();

        if (resultSet->next()) {
            int count = resultSet->getInt(1);
            delete resultSet;
            delete pstmt;
            return count > 0;
        }

        delete resultSet;
        delete pstmt;
    } catch (const sql::SQLException& e) {
        cerr << "SQL Error: " << e.what() << "\n";
    }

    return false;
}



void signUp(sql::Connection* con, const string& username, const string& password, bool isAdmin)
{
try
 {
    sql::PreparedStatement* pstmt;
    string tableName = isAdmin ? "admins" : "clients";
    pstmt = con->prepareStatement("INSERT INTO " + tableName + " (username, password) VALUES (?, ?)");
    pstmt->setString(1, username);
    pstmt->setString(2, password);
    pstmt->execute();
    cout << "USER SIGNED UP SUCCESSFULLY AS " << (isAdmin ? "admin" : "client") << "!" << endl;
    delete pstmt;
 }
 catch (sql::SQLException &e)
 {
   cout << "SQL Error: " << e.what() << endl;
 }
}

void csignUp(sql::Connection* con, const string& username, const string& password,const string& address,const long long phonenum, bool isAdmin)
{
try
 {
    sql::PreparedStatement* pstmt;
    string tableName = isAdmin ? "admins" : "clients";
    pstmt = con->prepareStatement("INSERT INTO " + tableName + " (username, password,address,phonenum) VALUES (?, ?,?,?)");
    pstmt->setString(1, username);
    pstmt->setString(2, password);
    pstmt->setString(3, address);
    pstmt->setInt64(4,phonenum);
    pstmt->execute();
    cout << "USER SIGNED UP SUCCESSFULLY AS " << (isAdmin ? "admin" : "client") << "!" << endl;
    delete pstmt;
 }
 catch (sql::SQLException &e)
 {
   cout << "SQL Error: " << e.what() << endl;
 }
}



bool login(sql::Connection* con, const string& username, const string& password, bool isAdmin)
{
  try
  {
    sql::PreparedStatement* pstmt;
    sql::ResultSet* res;
    string tableName = isAdmin ? "admins" : "clients";
    pstmt = con->prepareStatement("SELECT * FROM " + tableName + " WHERE username = ? AND password = ?");
    pstmt->setString(1, username);
    pstmt->setString(2, password);
    res = pstmt->executeQuery();
    bool success = res->next();
    delete res;
    delete pstmt;
    return success;
  }
  catch (sql::SQLException &e)
  {
    cout << "SQL Error: " << e.what() << endl;
    return false;
  }
}




int main()
{
  sql::Connection* con = establishDBConnection();
    ClientOperations Client(con);
    AdminOperations Admin(con);
  int role;
   cout << "                         REDDITUS : CAR RENTAL SYSTEM                           \n";
  cout << "================================================================================\n";
  cout << "                               SELECT YOUR ROLE                                 \n";
  cout << "================================================================================\n";
  cout << "                                  1. ADMIN                                      \n";
  cout << "                                  2. CUSTOMER                                   \n";
  cout << "\t ENTER YOUR ROLE : ";
  while (!(cin >> role)) {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Invalid input. Please enter a valid number: ";
}
  cout<<endl;
  bool isAdminLoggedIn, isClientLoggedIn;

  sql::Statement* stmt = con->createStatement();
  string createTableQuery = "CREATE TABLE IF NOT EXISTS cars (id INT AUTO_INCREMENT PRIMARY KEY, make VARCHAR(50), model VARCHAR(50), year INT,booked BOOLEAN default FALSE)";
  stmt->execute(createTableQuery);
  delete stmt;

  sql::Statement* stmt2 = con->createStatement();
  createTableQuery = "CREATE TABLE IF NOT EXISTS potentialcars (id INT AUTO_INCREMENT PRIMARY KEY, make VARCHAR(50), model VARCHAR(50), year INT,booked BOOLEAN default FALSE)";
  stmt->execute(createTableQuery);
  delete stmt2;

  sql::Statement* stmt1 = con->createStatement();
  createTableQuery = "CREATE TABLE IF NOT EXISTS transactions (id INT AUTO_INCREMENT PRIMARY KEY,car_id INT,customer_name VARCHAR(50),dob DATE,eob DATE,returnedstatus BOOLEAN default FALSE)";
  stmt1->execute(createTableQuery);
  delete stmt1;

  auto now = chrono::system_clock::now();
    time_t currentTime = chrono::system_clock::to_time_t(now);
    tm* localTime = localtime(&currentTime);
    int currentYear = localTime->tm_year + 1900;

  string adminUsername , adminPassword , clientUsername ,clientPassword;

  if(role == 1)
  {
      cout << "                             ADMIN AUTHENTICATION                               \n";
      cout << "================================================================================\n";
      sql::Statement* stmt = con->createStatement();
      string createTableQuery = "CREATE TABLE IF NOT EXISTS admins(username VARCHAR(50), password VARCHAR(65))";
      stmt->execute(createTableQuery);
      delete stmt;
      cout<<"\t ENTER ADMIN USERNAME : ";
      cin>>adminUsername;
      
      cout<<"\n\t ENTER ADMIN PASSWORD : ";
       cin.ignore(numeric_limits<streamsize>::max(), '\n');
       disableEcho();

    char c;
    while ((c = getchar()) != '\n') {
        cout << '*';
        adminPassword += c;  
    }
    string adminHashedPassword = sha256(adminPassword);
    enableEcho();
      isAdminLoggedIn = login(con, adminUsername, adminHashedPassword, true);
      if (isAdminLoggedIn)
      {
          cout << "\n                      ADMIN AUTHENTICATION SUCCESSFULL                          \n";
         
          int loop=1;
          do{
              int choosevaradmin;
              cout << "================================================================================\n";
              cout << "                             ADMIN OPERATIONS                                   \n";
              cout<<"\t 1. ADD NEW CAR"<<endl;
              cout<<"\t 2. REMOVE EXISTING CAR"<<endl;
              cout<<"\t 3. UPDATE EXISTING CAR DETAILS"<<endl;
              cout<<"\t 4. LIST ALL TRANSACTIONS"<<endl;
              cout<<"\t 5. POTENTIAL CARS APPROVAL/DENIAL"<<endl;
              cout<<"\t 6. EXIT"<<endl;
              cout<<"\t CHOOSE OPERATION : ";
              while (!(cin >> choosevaradmin)) {
                 cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a valid number: ";
            }
              cout <<endl;

              string make,model;
              int year;
              int carId;
              switch(choosevaradmin)
              {
                  case 1:
                  cout << "================================================================================\n";
                      cout << "\n                      ENTER CAR DETAILS                          \n";
                      cout << "================================================================================\n";
                      cout<<"\t ENTER CAR MAKE  : ";
                      cin>>make;
                      cout<<"\t ENTER CAR MODEL : ";
                      cin>>model;
                      cout<<"\t ENTER CAR YEAR  : ";
                      while (!(cin >> year) || year > currentYear) {
                      cin.clear();
                      cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
                      if (year > currentYear) {
                      cout << "Invalid input. The car's model year cannot be in the future. Please enter a valid year: ";
                      } else {
                      cout << "Invalid input. Please enter a valid number: ";
                      }
                    }
                      cout<<endl;
                      Admin.addCar(make, model, year);
                  break;
                  case 2:
                  cout << "================================================================================\n";
                      cout << "\n                      REMOVE EXISTING CAR                         \n";
                      cout << "================================================================================\n";
                      Admin.showAvailableCars();
                      cout<<"\t ENTER CAR ID : ";
                      while (!(cin >> carId)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input. Please enter a valid number: ";
                    }
                      cout<<endl;
                      Admin.removeCar(carId);
                  break;
                  case 3:  
                  cout << "================================================================================\n";               
                      cout << "\n                  UPDATE EXISTING CAR DETAILS                         \n";
                      cout << "================================================================================\n";
                      Admin.showAvailableCars();
                      cout<<"\t ENTER CAR ID    : ";
                      while (!(cin >> carId)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input. Please enter a valid number: ";
                    }
                      cout<<"\t ENTER CAR MAKE  : ";
                      cin>>make;
                      cout<<"\t ENTER CAR MODEL : ";
                      cin>>model;
                      cout<<"\t ENTER CAR YEAR  : ";
                      while (!(cin >> year) || year > currentYear) {
                      cin.clear();
                      cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
                      if (year > currentYear) {
                      cout << "Invalid input. The car's model year cannot be in the future. Please enter a valid year: ";
                      } else {
                      cout << "Invalid input. Please enter a valid number: ";
                      }
                      }
                      cout<<endl;
                      Admin.updateCar(carId, make, model, year);
                  break;
                  case 4:
                     cout << "================================================================================\n";
                      cout << "\n                  LISTING ALL TRANSACTIONS                         \n";
                      cout << "================================================================================\n";
                      Admin.viewAllTransactions();
                      cout<<endl;
                  break;
                  case 5:cout << "================================================================================\n";
                      cout << "\n                 POTENTIAL CARS APPROVAL/DENIAL                     \n";
                      cout << "================================================================================\n";
                      Admin.viewPotentialCars();
                  case 6:
                      loop=0;
                  break;
                  default:
                  cout << "================================================================================\n";
                      cout<<"\t ENTER VALID OPERATION ";
                      cout << "================================================================================\n";
              }
          }while(loop==1);
      }
      else
      {
          cout << "================================================================================\n";
          cout << "\n                        ADMIN AUTHENTICATION FAILED                           \n";
          cout << "================================================================================\n";
      }
  }
  else if(role == 2)
  {
      int log ;
      cout << "================================================================================\n";
      cout << "                             CLIENT AUTHENTICATION                               \n";
      cout << "================================================================================\n";
      cout<<"\t 1. LOGIN"<<endl;
      cout<<"\t 2. SIGN UP"<<endl;
      cout<<"\t CHOOSE MODE :"<<endl;
      while (!(cin >> log)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Invalid input. Please enter a valid number: ";
      }
      cout<<"\n";
      sql::Statement* stmt = con->createStatement();
      string createTableQuery = "CREATE TABLE IF NOT EXISTS clients(username VARCHAR(50), password VARCHAR(65), address VARCHAR(100), phonenum BIGINT)";
      stmt->execute(createTableQuery);
      delete stmt;
      string address;
      long long phnum;
      if(log == 1)
      {
          cout<<"\t ENTER CLIENT USERNAME : ";          
          cin>>clientUsername;
          cout<<"\t ENTER CLIENT PASSWORD : ";          
          cin.ignore(numeric_limits<streamsize>::max(), '\n');
       disableEcho();

    char c;
    while ((c = getchar()) != '\n') {
        cout << '*';
        clientPassword += c; 
    }

    enableEcho();
          cout<<"\n";
          string clientHashedPassword = sha256(clientPassword);
          isClientLoggedIn = login(con, clientUsername, clientHashedPassword, false);
      }
else {
    while (true) {
        cout << "\t SET CLIENT USERNAME : ";
        cin >> clientUsername;
        if (checkUsernameExists(con, clientUsername)) {
            cout << "Username already exists. Choose a different username.\n";
        } else {
            break; 
        }
    }

    cout << "\t SET CLIENT PASSWORD : ";
    cin >> clientPassword;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "\t ENTER YOUR ADDRESS : ";
    getline(cin, address);

while (true) {
        cout << "\t ENTER YOUR PHONE NUMBER (10 digits): ";
        if (!(cin >> phnum) || to_string(phnum).length() != 10) {
            cout << "Invalid input. Please enter a 10-digit number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } else {
            break; 
        }
    }

    cout<< phnum;
    cout << "\n";
    string clientHashedPassword = sha256(clientPassword);
    csignUp(con, clientUsername, clientHashedPassword, address, phnum, false); // Sign up a client
}







      if (isClientLoggedIn) {
       cout << "================================================================================\n";
      cout << "\n                      CLIENT AUTHENTICATION SUCCESSFULL                          \n";
      int loop=1;
      do
      {
          int choosevarclient;
          cout << "================================================================================\n";
          cout << "                             CLIENT OPERATIONS                                   \n";
          cout<<"\t 1. BOOK A CAR"<<endl;
          cout<<"\t 2. VIEW CARS WHICH WHERE BORROWED"<<endl;
          cout<<"\t 3. RETURN A CAR WHICH WAS BORROWED"<<endl;
          cout<<"\t 4. POST A CAR FOR RENT INTO THE APPLICATION"<<endl;
          cout<<"\t 5. SEE ALL TRANSACTION MADE PRIOR"<<endl;
          cout<<"\t 6. EXIT"<<endl;
          cout<<"\t CHOOSE OPERATION : ";
          while (!(cin >> choosevarclient)) {
          cin.clear();
          cin.ignore(numeric_limits<streamsize>::max(), '\n');
          cout << "Invalid input. Please enter a valid number: ";
          }
          cout<<"\n";
          string make,model;
          int year;
          int carId;
          int returnstatus;
          string dob,eob;
          switch(choosevarclient)
          {
              case 1:
                      cout << "================================================================================\n";
                      cout << "\n                      BOOK A CAR                        \n";
                      cout << "================================================================================\n";
                      
                  Client.showAvailableCars();


                  Client.bookCar(clientUsername);
                  
                  break;
              case 2: 
                      cout << "================================================================================\n";
                      cout << "\n                NOT RETURNED CAR DETAILS                          \n";
                      cout << "================================================================================\n";
                      
                    returnstatus=Client.NotReturned(clientUsername);
                  break;
              case 3: cout << "================================================================================\n";
                      cout << "\n                     RETURN A CAR                           \n";
                      cout << "================================================================================\n";
                      
               returnstatus=Client.NotReturned(clientUsername);
               if(returnstatus==1){
                  
                  Client.returnCar(clientUsername);
                  
               }
               break;
              case 4:
                  cout << "================================================================================\n";
                      cout << "\n                      ENTER CAR DETAILS FOR POST                         \n";
                      cout << "================================================================================\n";
                      cout<<"\t ENTER CAR MAKE  : ";
                      cin>>make;
                      cout<<"\t ENTER CAR MODEL : ";
                      cin>>model;
                      cout<<"\t ENTER CAR YEAR  : ";
                      while (!(cin >> year) || year > currentYear) {
                      cin.clear();
                      cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
                      if (year > currentYear) {
                        cout << "Invalid input. The car's model year cannot be in the future. Please enter a valid year: ";
                      } else {
                        cout << "Invalid input. Please enter a valid number: ";
                      }
                      }
                      cout<<endl;
                      Client.postCarForRent( make, model,year);
                  break;
              case 5:Client.displayClientTransactions(clientUsername);
                  break;
              case 6:loop=0;
                  break;
              default:
                  cout<<"\t ENTER VALID OPERATION";
          }

     }while(loop==1);

  }
  else
  {
          cout << "\n================================================================================\n";
          cout << "\n                        CLIENT AUTHENTICATION FAILED                           \n";
          cout << "================================================================================\n";
  }

  }
  else
  {
    cout<<"INVALID INPUT\n\n\n";
    main();
  }
  // Close the connection
  con->close();
  delete con;
  return 0;
}


