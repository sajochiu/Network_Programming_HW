#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <arpa/inet.h>

struct status{
    bool logged_in;
    int uid;
};

// global variables
int result_count = 0;
int result_uid = 0;
std::string result_str = "";

int str_arr_iter = 0;
std::string result_str_arr[100];

std::string DATE = "4/26";
std::string DATE_with_year = "2020-04-26";

// [[maybe_unused]]

// functions
//void error(const char *msg) __attribute__((noreturn));
[[ noreturn ]] void error(const char *msg);
inline bool exists_test(const std::string& name);

int parse_the_command(char * buffer, int client_sockfd, struct status * client_status);
int read_line(int client_sockfd, char * buffer, int buffer_size);
void serve_process(int client_sockfd);
void print_welcome_message(int client_sockfd);

void whoami(int client_sockfd, struct status * client_status);
void login(int client_sockfd, struct status * client_status, std::string s[]);
void logout(int client_sockfd, struct status * client_status);
void my_register(int client_sockfd, struct status * client_status, std::string s[]);

void create_board(int client_sockfd, struct status * client_status, std::string s[]);
void create_post(int client_sockfd, struct status * client_status, std::string s[], int arg_count);
void list_board(int client_sockfd, struct status * client_status, std::string s[], bool search);
void list_post(int client_sockfd, struct status * client_status, std::string s[], bool search);
void my_read(int client_sockfd, struct status * client_status, std::string s[]);
void delete_post(int client_sockfd, struct status * client_status, std::string s[]);
void update_post(int client_sockfd, struct status * client_status, std::string s[], int arg_count, int choice);
void comment(int client_sockfd, struct status * client_status, std::string s[], int arg_count);

bool valid_format_for_create_post(std::string s[], int arg_count);

static int callback_whoami(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_login_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_login_2(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_logout(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_register_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_register_2(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_create_board_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_create_board_2(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_create_board_3(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_list_board(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_create_post_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_create_post_2(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_create_post_3(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_list_post_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_list_post_2(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_my_read_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_my_read_2(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_delete_post_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_delete_post_2(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_delete_post_3(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_delete_post_4(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_update_post_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_update_post_2(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_update_post_3(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_update_post_4(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_comment_1(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_comment_2(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_comment_3(void *NotUsed, int argc, char **argv, char **szColName);
static int callback_comment_4(void *NotUsed, int argc, char **argv, char **szColName);

static int callback_createdb(void *NotUsed, int argc, char **argv, char **szColName);

bool valid_format_for_create_post(std::string s[], int arg_count){
    /* 
     * there must be at least 6 arguments
     * s[2] must be "--title"
     * "--content" must be after s[3]
     * there must be at least one string after "--content"
     */
    int content_index = -1;
    if(arg_count < 6){
        return false;
    }
    if(s[2] != "--title"){
        return false;
    }
    if(s[3] == "--content"){
        return false;
    }
    // find the index of "--content""
    for(int i=0; i<arg_count; i++){
        if(s[i] == "--content"){
            content_index = i;
        }
    }
    if(content_index == -1){
        return false;
    }
    if(content_index == (arg_count-1) ){
        return false;
    }
    return true;
}

bool valid_format_for_update_post(std::string s[], int arg_count){
    /* 
     * there must be at least 4 arguments
     * s[2] must be "--title" or "--content"
     * there must be at least one string after s[2]
     */
    int index = -1;
    if(arg_count < 4){
        return false;
    }
    if( (s[2] != "--title") && (s[2] != "--content") ){
        return false;
    }
    return true;
}

int main(int argc, char ** argv){   // argv[1] is the port number

    // database setting
    sqlite3 *db;    // database
	char *szErrMsg = 0; // store the database error message
    int rc; // return value

    [[gnu::unused]] std::string string_buffer;
    int server_sockfd;  // fd of the server
    int client_sockfd;  // fd of the accepted client
    int portno;         // 
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    clilen = sizeof(cli_addr);
    int n = 0;
    int pid = 0;

    // check if database USER is exist
    if(!exists_test("USERS.db")){
        rc = sqlite3_open("USERS.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        // **********************************************************************************************
        const char * sql_create_db = "CREATE TABLE USERS(UID INTEGER PRIMARY KEY AUTOINCREMENT, Username TEXT NOT NULL UNIQUE, Email TEXT NOT NULL, Password TEXT NOT NULL)";
        std::cout << "create database USER\n";
        rc = sqlite3_exec(db, sql_create_db, callback_createdb, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
        }
        if(db) sqlite3_close(db);
        // **********************************************************************************************

    }

    // check if database BOARD is exist
    if(!exists_test("BOARD.db")){
        rc = sqlite3_open("BOARD.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        // **********************************************************************************************
        const char * sql_create_db = "CREATE TABLE BOARD(BoardID INTEGER PRIMARY KEY AUTOINCREMENT, Boardname TEXT NOT NULL UNIQUE, Moderator TEXT NOT NULL)";
        std::cout << "create database BOARD\n";
        rc = sqlite3_exec(db, sql_create_db, callback_createdb, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
        }
        if(db) sqlite3_close(db);
        // **********************************************************************************************
    }

    // check if database POST is exist
    if(!exists_test("POST.db")){
        rc = sqlite3_open("POST.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        // **********************************************************************************************
        const char * sql_create_db = "CREATE TABLE POST(PostID INTEGER PRIMARY KEY AUTOINCREMENT, Title TEXT NOT NULL, Author TEXT NOT NULL, Date TEXT NOT NULL, Content TEXT NOT NULL, Boardname TEXT NOT NULL)";
        std::cout << "create database POST\n";
        rc = sqlite3_exec(db, sql_create_db, callback_createdb, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
        }
        if(db) sqlite3_close(db);
        // **********************************************************************************************

    }
    
    
    // check if the port is provided
    if(argc < 2){
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // create the socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
    if(server_sockfd < 0){
        error("ERROR opening socket");
    }

    // clear the address struct
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // get the port number in integer
    portno = atoi(argv[1]);

    // set the address struct
    serv_addr.sin_family = AF_INET;
    //serv_addr.sin_addr.s_addr = inet_addr("255.127.63.31");
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portno);

    // bind the socket to the specified address and port
    if(bind(server_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR on binding");
    }

    // listen
    listen(server_sockfd, 15);

    
    // waiting for new client
    while(1){
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(client_sockfd < 0) error("ERROR on accept");
        pid = fork();
        if(pid < 0){
            error("ERROR on accept");
        }
        if(pid == 0){
            close(server_sockfd);
            serve_process(client_sockfd);
            exit(0);
        }else{
            close(client_sockfd);
        }
    }
    while(1);
    close(client_sockfd);
    close(server_sockfd);
    return 0;
}

void print_welcome_message(int client_sockfd){
    std::cout << "New connection.\n";
    int n = write(client_sockfd, "********************************\n", 33);
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, "** Welcome to the BBS server. **\n", 33);
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, "********************************\n", 33);
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, "% ", 2);
    if(n < 0) error("ERROR writing to socket");
}

void serve_process(int client_sockfd){
    
    int n = 0;
    char buffer[256];

    // database
    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;

    // status for each client
    struct status client_status;
    client_status.logged_in = false;
    client_status.uid = 0;
    print_welcome_message(client_sockfd);

    // read the message from client
    while(1){
        bzero(buffer, 256);
        n = read_line(client_sockfd, buffer, 255);
        if(n < 0) error("ERROR reading from socket");
        if(parse_the_command(buffer, client_sockfd, &client_status) == -1){
            break;
        }
        // prompt
        n = write(client_sockfd, "% ", 2);
        if(n < 0) error("ERROR writing to socket");
    }
}

void whoami(int client_sockfd, struct status * client_status){
    
    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    if(client_status->logged_in){

        // prepare the sql command
        // *************************************************************************************
        std::string sql_command = "SELECT * FROM USERS WHERE UID = ";
        sql_command += std::to_string(client_status->uid);
        std::cout << sql_command << "\n";
        // open database
        rc = sqlite3_open("USERS.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        rc = sqlite3_exec(db, sql_command.c_str(), callback_whoami, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
            exit(-1);
        }
        if(db){
            sqlite3_close(db);
        }
        result_str += "\n";
        const char * name = result_str.c_str();
        n = write(client_sockfd, name, strlen(name));
        if(n < 0) error("ERROR writing to socket");
    }else{
        n = write(client_sockfd, "Please login first.\n", 21);
        if(n < 0) error("ERROR writing to socket");
    }
}

void login(int client_sockfd, struct status * client_status, std::string s[]){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    // check if the client has logged in
    if(client_status->logged_in){
        n = write(client_sockfd, "Please logout first.\n", 22);
        if(n < 0) error("ERROR writing to socket");
    }else{

        // check if the user exists
        // prepare the sql command
        // *************************************************************************************
        std::string sql_command = "SELECT COUNT(*) FROM USERS WHERE Username = '" + s[1] + "' AND Password = '" + s[2] + "'";
        std::cout << sql_command << "\n";
        // open database
        rc = sqlite3_open("USERS.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        rc = sqlite3_exec(db, sql_command.c_str(), callback_login_1, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
            exit(-1);
        }
        if(db){
            sqlite3_close(db);
        }

        // if the username really exists and the corresponding password is correct
        if(result_count == 1){

            client_status->logged_in = true;
            // prepare the sql command
            // *************************************************************************************
            std::string sql_command = "SELECT * FROM USERS WHERE Username = '" + s[1] + "'";
            std::cout << sql_command << "\n";
            // open database
            rc = sqlite3_open("USERS.db", &db);
            if(rc){
                std::cout << "cannot open database\n";
                exit(-1);
            }
            rc = sqlite3_exec(db, sql_command.c_str(), callback_login_2, 0, &szErrMsg);
            if(rc != SQLITE_OK){
                std::cout << "SQL Error: " << szErrMsg << std::endl;
                sqlite3_free(szErrMsg);
                exit(-1);
            }
            if(db){
                sqlite3_close(db);
            }
            client_status->uid = result_uid;
            std::string message = "Welcome, " + s[1] + ".\n";
            const char * name = message.c_str();

            n = write(client_sockfd, name, strlen(name));
            if(n < 0) error("ERROR writing to socket");

        }else{

            n = write(client_sockfd, "Login failed.\n", 15);
            if(n < 0) error("ERROR writing to socket");
        }
    }
}

void logout(int client_sockfd, struct status * client_status){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    if(client_status->logged_in){

        client_status->logged_in = false;

        // prepare the sql command
        // *************************************************************************************
        std::string sql_command = "SELECT * FROM USERS WHERE UID = " + std::to_string(client_status->uid);
        std::cout << sql_command << "\n";
        // open database
        rc = sqlite3_open("USERS.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        rc = sqlite3_exec(db, sql_command.c_str(), callback_logout, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
            exit(-1);
        }
        if(db){
            sqlite3_close(db);
        }
        std::string message = "Bye, " + result_str + "\n";
        const char * name = message.c_str();
        n = write(client_sockfd, name, strlen(name));
        if(n < 0) error("ERROR writing to socket");

    }else{

        n = write(client_sockfd, "Please login first.\n", 21);
        if(n < 0) error("ERROR writing to socket");

    }
}

void my_register(int client_sockfd, struct status * client_status, std::string s[]){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    // prepare the sql command
    // *************************************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM USERS WHERE Username = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("USERS.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_register_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }

    if(result_count > 0){   // check if the username is already be registered

        n = write(client_sockfd, "Username is already used.\n", 27);
        if(n < 0) error("ERROR writing to socket");

    }else{  // register with the user information

        // *************************************************************************************
        sql_command = "INSERT INTO USERS(Username, Email, Password) VALUES ('" + s[1] + "', '" + s[2] + "', '" + s[3] + "')";
        std::cout << sql_command << "\n";
        // open database
        rc = sqlite3_open("USERS.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        rc = sqlite3_exec(db, sql_command.c_str(), callback_register_2, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
            exit(-1);
        }
        if(db){
            sqlite3_close(db);
        }
        n = write(client_sockfd, "Register successfully.\n", 24);
        if(n < 0) error("ERROR writing to socket");
    }
}

void create_board(int client_sockfd, struct status * client_status, std::string s[]){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    if(!client_status->logged_in){

        n = write(client_sockfd, "Please login first.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // prepare the sql command
    // ********************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM BOARD WHERE Boardname = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("BOARD.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_create_board_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }

    if(result_count > 0){   // check if the boardname is already be used

        n = write(client_sockfd, "Board is already exist.\n", 25);
        if(n < 0) error("ERROR writing to socket");

    }else{  // add new board

        // ********************************************************************
        sql_command = "SELECT * FROM USERS WHERE UID = ";
        sql_command += std::to_string(client_status->uid);
        std::cout << sql_command << "\n";
        // open database
        rc = sqlite3_open("USERS.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        rc = sqlite3_exec(db, sql_command.c_str(), callback_create_board_2, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
            exit(-1);
        }
        if(db){
            sqlite3_close(db);
        }
        std::string mod = result_str;
        // ********************************************************************
        sql_command = "INSERT INTO BOARD(Boardname, Moderator) VALUES ('" + s[1] + "', '" + mod + "')";
        std::cout << sql_command << "\n";
        // open database
        rc = sqlite3_open("BOARD.db", &db);
        if(rc){
            std::cout << "cannot open database\n";
            exit(-1);
        }
        rc = sqlite3_exec(db, sql_command.c_str(), callback_create_board_3, 0, &szErrMsg);
        if(rc != SQLITE_OK){
            std::cout << "SQL Error: " << szErrMsg << std::endl;
            sqlite3_free(szErrMsg);
            exit(-1);
        }
        if(db){
            sqlite3_close(db);
        }
        n = write(client_sockfd, "Create board successfully.\n", 28);
        if(n < 0) error("ERROR writing to socket");
    }
}
void create_post(int client_sockfd, struct status * client_status, std::string s[], int arg_count){
    //int n;
    //n = write(client_sockfd, "This is a valid command\n", 25);
    //if(n < 0) error("ERROR writing to socket");

    std::cout << arg_count << "\n";

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    int index_of_content = -1;
    int iter = 0;

    if(!client_status->logged_in){

        n = write(client_sockfd, "Please login first.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // check if the board is exist
    // **********************************************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM BOARD WHERE Boardname = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("BOARD.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_create_post_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    if(result_count == 0){  // the board doesn't exist

        n = write(client_sockfd, "Board is not exist.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    for(int i=0; i<arg_count; i++){
        std::cout << s[i] << " ";
    }

    // find the index of content in the command
    while(index_of_content < 0){
        if(s[iter] == "--content"){
            index_of_content = iter;
        }
        iter++;
    }

    std::cout << "index_of_content: " << index_of_content << "\n";

    // build the string title and the string content
    std::string title = "";
    title.clear();
    std::string content = "";
    content.clear();
    for(int i=3; i<=(index_of_content-1); i++){
        title += s[i];
        title += " ";
    }

    content = "--\n";
    for(int i=index_of_content+1; i<arg_count; i++){
        content += s[i];
        content += " ";
    }
    content += "\n--\n";

    // **********************************************************************************************
    sql_command = "SELECT * FROM USERS WHERE UID = ";
    sql_command += std::to_string(client_status->uid);
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("USERS.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_create_post_2, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************
    std::string author = result_str;

    std::string boardname = s[1];

    // **********************************************************************************************
    sql_command = "INSERT INTO POST(Title, Author, Date, Content, Boardname) VALUES ('" + title + "', '" + author + "', '" + DATE + "', '" + content + "', '" + boardname + "')";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_create_post_3, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    n = write(client_sockfd, "Create post successfully.\n", 27);
    if(n < 0) error("ERROR writing to socket");
    // **********************************************************************************************

}
void list_board(int client_sockfd, struct status * client_status, std::string s[], bool search){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;
    
    std::string search_substr;
    if(search){
        search_substr = s[1].substr(2, s[1].length()-2);
    }

    for(int i=0; i<100; i++){
        result_str_arr[i].clear();
    }
    str_arr_iter = 0;
    // **********************************************************************************************
    std::string sql_command = "SELECT * FROM BOARD";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("BOARD.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_list_board, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    n = write(client_sockfd, "Index     Name     Moderator\n", 30);
    if(n < 0) error("ERROR writing to socket");

    for(int i=0; i<=str_arr_iter; i++){
        result_str_arr[i] += "\n";
        std::cout << result_str_arr[i];
        if(search){
            if( result_str_arr[i].find(search_substr) != std::string::npos ) {
                n = write(client_sockfd, result_str_arr[i].c_str(), result_str_arr[i].length());
                if(n < 0) error("ERROR writing to socket");
            }
        }else{
            n = write(client_sockfd, result_str_arr[i].c_str(), result_str_arr[i].length());
            if(n < 0) error("ERROR writing to socket");
        }
        
    }
}
void list_post(int client_sockfd, struct status * client_status, std::string s[], bool search){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;
    std::string search_substr;
    if(search){
        search_substr = s[2].substr(2, s[2].length()-2);
    }

    // check if the board is exist
    // **********************************************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM BOARD WHERE Boardname = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("BOARD.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_list_post_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    if(result_count == 0){  // the board doesn't exist

        n = write(client_sockfd, "Board is not exist.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    for(int i=0; i<100; i++){
        result_str_arr[i].clear();
    }
    str_arr_iter = 0;
    // **********************************************************************************************
    sql_command = "SELECT * FROM POST WHERE Boardname = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_list_post_2, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    
    n = write(client_sockfd, "ID    Title        Author    Date\n", 35);
    if(n < 0) error("ERROR writing to socket");

    for(int i=0; i<=str_arr_iter; i++){
        result_str_arr[i] += "\n";
        std::cout << result_str_arr[i];
        if(search){
            if( result_str_arr[i].find(search_substr) != std::string::npos ) {
                n = write(client_sockfd, result_str_arr[i].c_str(), result_str_arr[i].length());
                if(n < 0) error("ERROR writing to socket");
            }
        }else{
            n = write(client_sockfd, result_str_arr[i].c_str(), result_str_arr[i].length());
            if(n < 0) error("ERROR writing to socket");
        }
    }

}
void my_read(int client_sockfd, struct status * client_status, std::string s[]){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    // check if the post exists
    // **********************************************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_my_read_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************
    
    if(result_count == 0){

        n = write(client_sockfd, "Post is not exist.\n", 20);
        if(n < 0) error("ERROR writing to socket");
        
        return;
    }

    // find the post
    // **********************************************************************************************
    sql_command = "SELECT * FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_my_read_2, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    std::cout << result_str_arr[0] << " " << result_str_arr[1] << " " << result_str_arr[2] << " " <<result_str_arr[3] << "\n";

    // write Author
    n = write(client_sockfd, "Author: ", 9);
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, result_str_arr[0].c_str(), result_str_arr[0].length());
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, "\n", 2);
    if(n < 0) error("ERROR writing to socket");

    // write Title
    n = write(client_sockfd, "Title: ", 8);
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, result_str_arr[1].c_str(), result_str_arr[1].length());
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, "\n", 2);
    if(n < 0) error("ERROR writing to socket");

    // write Date
    n = write(client_sockfd, "Date: ", 7);
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, DATE_with_year.c_str(), DATE_with_year.length());
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, "\n", 2);
    if(n < 0) error("ERROR writing to socket");

    // write Content
    
    // replace <br> with "\n"
    std::string replaceby = "\n";
    n = 0;
    while( (n = result_str_arr[3].find("<br>", n)) != std::string::npos ){
        result_str_arr[3].replace(n, 4, replaceby);
        n += replaceby.length();
    }
    n = write(client_sockfd, result_str_arr[3].c_str(), result_str_arr[3].length());
    if(n < 0) error("ERROR writing to socket");
    n = write(client_sockfd, "\n", 2);
    if(n < 0) error("ERROR writing to socket");

}
void delete_post(int client_sockfd, struct status * client_status, std::string s[]){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    // "Please login first"
    if(!client_status->logged_in){
        
        n = write(client_sockfd, "Please login first.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // "Post is not exist"
    // **********************************************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    if(result_count == 0){

        n = write(client_sockfd, "Post is not exist.\n", 20);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // "Not the post owner"
    // **********************************************************************************************
    sql_command = "SELECT * FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_2, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    std::string result_str1 = result_str;

    // **********************************************************************************************
    sql_command = "SELECT * FROM USERS WHERE UID = '" + std::to_string(client_status->uid) + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("USERS.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_3, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }

    // **********************************************************************************************

    std::string result_str2 = result_str;

    if(result_str1 != result_str2){

        n = write(client_sockfd, "Not the post owner.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // "Delete sucessfully"
    // n = write(client_sockfd, "You have the access to delete this post.\n", 42);
    // if(n < 0) error("ERROR writing to socket");

    // **********************************************************************************************
    sql_command = "DELETE FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_4, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************
    n = write(client_sockfd, "Delete sucessfully.\n", 21);
    if(n < 0) error("ERROR writing to socket");
    
}
void update_post(int client_sockfd, struct status * client_status, std::string s[], int arg_count, int choice){
    
    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    // "Please login first"
    if(!client_status->logged_in){
        
        n = write(client_sockfd, "Please login first.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // "Post is not exist"
    // **********************************************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    if(result_count == 0){

        n = write(client_sockfd, "Post is not exist.\n", 20);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // "Not the post owner"
    // **********************************************************************************************
    sql_command = "SELECT * FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_2, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    std::string result_str1 = result_str;

    // **********************************************************************************************
    sql_command = "SELECT * FROM USERS WHERE UID = '" + std::to_string(client_status->uid) + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("USERS.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_3, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }

    // **********************************************************************************************

    std::string result_str2 = result_str;

    if(result_str1 != result_str2){

        n = write(client_sockfd, "Not the post owner.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    std::string update_str = "";
    update_str.clear();
    if(choice == 2){
        update_str = "--\n";
    }
    for(int i=3; i<arg_count; i++){
        update_str = update_str + s[i] + " ";
    }
    if(choice == 2){
        update_str += "\n--\n";
    }
    
    // "Update sucessfully"
    // n = write(client_sockfd, "You have the access to update this post.\n", 42);
    // if(n < 0) error("ERROR writing to socket");

    // **********************************************************************************************
    sql_command = "UPDATE POST SET ";
    if(choice == 1){
        sql_command += " Title = '";
    }else if(choice == 2){
        sql_command += " Content = '";
    }
    sql_command = sql_command + update_str + "' WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_delete_post_4, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************
    n = write(client_sockfd, "Update sucessfully.\n", 21);
    if(n < 0) error("ERROR writing to socket");
    
}
void comment(int client_sockfd, struct status * client_status, std::string s[], int arg_count){

    sqlite3 *db;
	char *szErrMsg = 0;
    int rc;
    int n;

    // "Please login first"
    if(!client_status->logged_in){
        
        n = write(client_sockfd, "Please login first.\n", 21);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // "Post is not exist"
    // **********************************************************************************************
    std::string sql_command = "SELECT COUNT(*) FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_comment_1, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    if(result_count == 0){

        n = write(client_sockfd, "Post is not exist.\n", 20);
        if(n < 0) error("ERROR writing to socket");

        return;
    }

    // **********************************************************************************************
    sql_command = "SELECT * FROM POST WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_comment_2, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************
    std::string old_comment = result_str;
    // **********************************************************************************************
    sql_command = "SELECT * FROM USERS WHERE UID = ";
    sql_command += std::to_string(client_status->uid);
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("USERS.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_comment_3, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************
    std::string username = result_str;

    std::string comment_str;
    comment_str.clear();
    for(int i=2; i<arg_count; i++){
        comment_str = comment_str + s[i] + " ";
    }
    comment_str += "\n";

    old_comment = old_comment + username + " : " + comment_str;

    // **********************************************************************************************
    sql_command = "UPDATE POST SET ";
    sql_command += " Content = '";
    sql_command = sql_command + old_comment + "' WHERE PostID = '" + s[1] + "'";
    std::cout << sql_command << "\n";
    // open database
    rc = sqlite3_open("POST.db", &db);
    if(rc){
        std::cout << "cannot open database\n";
        exit(-1);
    }
    rc = sqlite3_exec(db, sql_command.c_str(), callback_comment_4, 0, &szErrMsg);
    if(rc != SQLITE_OK){
        std::cout << "SQL Error: " << szErrMsg << std::endl;
        sqlite3_free(szErrMsg);
        exit(-1);
    }
    if(db){
        sqlite3_close(db);
    }
    // **********************************************************************************************

    n = write(client_sockfd, "Comment sucessfully.\n", 21);
    if(n < 0) error("ERROR writing to socket");

}

int parse_the_command(char * buffer, int client_sockfd, struct status * client_status){

    int n = 0;
    std::string s[1000];

    // split tokens
    int arg_count=0; // number of the arguments
    char *test = strtok(buffer, " ");
    while(test != NULL){
        s[arg_count] = test;
        test = strtok(NULL, " ");
        arg_count++;
    }

    // decide the command
    if(s[0] == "exit"){ // done
        if(arg_count == 1){
            return -1;
        }else{
            n = write(client_sockfd, "Usage: exit\n", 13);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "whoami"){ // done
        if(arg_count == 1){
            whoami(client_sockfd, client_status);
        }else{
            n = write(client_sockfd, "Usage: whoami\n", 15);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "login"){  // done
        if(arg_count == 3){
            login(client_sockfd, client_status, s);
        }else{
            n = write(client_sockfd, "Usage: login <username> <password>\n", 36);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "logout"){ // done
        if(arg_count == 1){
            logout(client_sockfd, client_status);
        }else{
            n = write(client_sockfd, "Usage: logout\n", 15);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "register"){   // done
        if(arg_count == 4){
            my_register(client_sockfd, client_status, s);
        }else{
            n = write(client_sockfd, "Usage: register <username> <email> <password>\n", 47);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "create-board"){   //done
        if(arg_count == 2){
            create_board(client_sockfd, client_status, s);
        }else{
            n = write(client_sockfd, "Usage: create-board <name>\n", 28);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "create-post"){    // done
        if(valid_format_for_create_post(s, arg_count)){
            create_post(client_sockfd, client_status, s, arg_count);
        }else{
            n = write(client_sockfd, "Usage: create-post <board-name> --title <title> --content <content>\n", 69);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "list-board"){ // done
        if(arg_count == 1){
            list_board(client_sockfd, client_status, s, 0);
        }else if( (arg_count == 2) && (s[1].rfind("##", 0) == 0) ){
            list_board(client_sockfd, client_status, s, 1);
        }else{
            n = write(client_sockfd, "Usage: list-board ##<key>\n", 27);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "list-post"){  // done
        if(arg_count == 2){
            list_post(client_sockfd, client_status, s, 0);
        }else if( (arg_count == 3) && (s[2].rfind("##", 0) == 0) ){
            list_post(client_sockfd, client_status, s, 1);
        }else{
            n = write(client_sockfd, "Usage: list-post <board-name> ##<key>\n", 39);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "read"){   // NOT YET
        if(arg_count == 2){
            my_read(client_sockfd, client_status, s);
        }else{
            n = write(client_sockfd, "Usage: read <post-id>\n", 23);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "delete-post"){    // done
        if(arg_count == 2){
            delete_post(client_sockfd, client_status, s);
        }else{
            n = write(client_sockfd, "Usage: delete-post <post-id>\n", 30);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "update-post"){    // done
        if(valid_format_for_update_post(s, arg_count)){
            if(s[2] == "--title"){
                update_post(client_sockfd, client_status, s, arg_count, 1);
            }else if(s[2] == "--content"){
                update_post(client_sockfd, client_status, s, arg_count, 2);
            }
        }else{
            n = write(client_sockfd, "Usage: update-post <post-id> --title/content <new>\n", 52);
            if(n < 0) error("ERROR writing to socket");
        }
    }else if(s[0] == "comment"){
        if(arg_count >= 3){
            comment(client_sockfd, client_status, s, arg_count);
        }else{
            n = write(client_sockfd, "Usage: comment <post-id> <comment>\n", 36);
            if(n < 0) error("ERROR writing to socket");
        }
    }else{
        if(arg_count != 0){
            // the command is not provided
            n = write(client_sockfd, "idiot\n", 7);
            if(n < 0) error("ERROR writing to socket");
        }
    }

    return 1;

}

int read_line(int client_sockfd, char * buffer, int buffer_size){
	int iter = 0;
	read(client_sockfd, buffer, buffer_size);
    while(buffer[iter]!='\r' && buffer[iter]!='\n' && buffer[iter]!='\0'){
        iter++;
    }
    buffer[iter] = '\0';
	return iter;
}

static int callback_register_1(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_register_2(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
	return 0;
}
static int callback_login_1(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_login_2(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_uid = atoi(argv[0]);
	return 0;
}
static int callback_createdb(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
	return 0;
}
static int callback_whoami(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[1];
	return 0;
}
static int callback_logout(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[1];
	return 0;
}
static int callback_create_board_1(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_create_board_2(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[1];
	return 0;
}
static int callback_create_board_3(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
	return 0;
}


static int callback_list_board(void *NotUsed, int argc, char **argv, char **szColName){
    result_str_arr[str_arr_iter] = "";
    std::cout << "call back\n";
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
        result_str_arr[str_arr_iter] += argv[i];
        result_str_arr[str_arr_iter] += "      ";
	}
    str_arr_iter++;
	std::cout << "\n";

	return 0;
}
static int callback_create_post_1(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_create_post_2(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[1];
	return 0;
}
static int callback_create_post_3(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
	return 0;
}

static int callback_list_post_1(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_list_post_2(void *NotUsed, int argc, char **argv, char **szColName){
    result_str_arr[str_arr_iter] = "";
    std::cout << "call back\n";
    for(int i = 0; i < 4; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
        result_str_arr[str_arr_iter] += argv[i];
        result_str_arr[str_arr_iter] += "      ";
	}
    str_arr_iter++;
	std::cout << "\n";

    return 0;
}

static int callback_my_read_1(void *NotUsed, int argc, char **argv, char **szColName){
	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_my_read_2(void *NotUsed, int argc, char **argv, char **szColName){

	for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}

    result_str_arr[0] = argv[2];
    result_str_arr[1] = argv[1];
    result_str_arr[2] = argv[3];
    result_str_arr[3] = argv[4];

	std::cout << "\n";
	return 0;
}

static int callback_delete_post_1(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_delete_post_2(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[2];
	return 0;
}
static int callback_delete_post_3(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[1];
	return 0;
}
static int callback_delete_post_4(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
	return 0;
}

static int callback_update_post_1(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_update_post_2(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[2];
	return 0;
}
static int callback_update_post_3(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[1];
	return 0;
}
static int callback_update_post_4(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
	return 0;
}

static int callback_comment_1(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_count = atoi(argv[0]);
	return 0;
}
static int callback_comment_2(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[4];
	return 0;
}
static int callback_comment_3(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
    result_str = argv[1];
	return 0;
}
static int callback_comment_4(void *NotUsed, int argc, char **argv, char **szColName){
    for(int i = 0; i < argc; i++){
		std::cout << szColName[i] << " = " << argv[i] << std::endl;
	}
	std::cout << "\n";
	return 0;
}

inline bool exists_test(const std::string& name) {
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

void error(const char *msg){
    perror(msg);
    exit(1);
}
