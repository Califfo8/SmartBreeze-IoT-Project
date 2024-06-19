#---------------------------------------------
#               PARAMETERS
#---------------------------------------------
# List of nodes to compile
NODE_LIST="energy-manager climate-manager"
# MySQL Credentials for creating the database
MYSQL_USER="root"
MYSQL_PASSWORD="PASSWORD"
DATABASE_NAME="SmartBreezeDB" 
#---------------------------------------------

#Utility variables
SHELL_HYSTORY=""
BASE_PATH=$(pwd)

function compile_node(){
    cd ./$1
    make distclean
    make TARGET=nrf52840 BOARD=dongle $1
    cd ..
}

function shell_echo(){
    echo -e "$1"
    local param="$1"
    SHELL_HYSTORY="${SHELL_HYSTORY} ${param}"
}

function compile_all_nodes(){
    shell_echo "Inizio compilazione:\n"
    for node in $NODE_LIST
    do
        compile_node $node
        shell_echo "\t - ${node} compilato\n"
    done
    echo "Press any key to continue..."
    read -n 1 -s
    clear
    echo -e $SHELL_HYSTORY
    echo "Fine compilazione"
}

function run_cooja(){
    cd 
    cd contiki-ng/tools/cooja
    gnome-terminal -- ./gradlew run --info --debug
    cd $BASE_PATH
}

function run_rpl_border_router(){
    cd ..
    cd rpl-border-router
    gnome-terminal -- make TARGET=cooja connect-router-cooja
    cd $BASE_PATH
}

function run_CoAP_server(){
    cd ./CoAP_Server
    gnome-terminal -- python3 ./CoAP_Server.py
    cd $BASE_PATH
}

function mysql_cmd() {
    mysql -u$MYSQL_USER -p$MYSQL_PASSWORD -e "$1"
}

function create_db(){
    # Create database
    mysql_cmd "DROP DATABASE IF EXISTS ${DATABASE_NAME};"
    mysql_cmd "CREATE DATABASE IF NOT EXISTS ${DATABASE_NAME};"

    #Adding tables
    mysql_cmd "CREATE TABLE IF NOT EXISTS ${DATABASE_NAME}.nodes 
    (ip VARCHAR(50) NOT NULL,
    name VARCHAR(255) NOT NULL, 
    resource VARCHAR(255) NOT NULL,
    status VARCHAR(255) NOT NULL,
    PRIMARY KEY (ip));"
    echo "Database creato"
}

function query_db()
{
    mysql -u$MYSQL_USER -p$MYSQL_PASSWORD -D$DATABASE_NAME -e "$1"
}

case $1 in
    compile_all)
        compile_all_nodes
        ;;
    compile)
        compile_node $2
        ;;
    cooja)
        run_cooja
        ;;
    border-router)
        run_rpl_border_router
        ;;
    coap-server)
        run_CoAP_server
        ;;
    sim)
        compile_all_nodes
        shell_echo "Starting Cooja..."
        run_cooja
        shell_echo "Press any key to start the border-router..."
        read -n 1 -s
        run_rpl_border_router
        echo "Press any key to start the CoAP server..."
        read -n 1 -s
        run_CoAP_server
        ;;
    relsim)
        run_rpl_border_router
        echo "Press any key to start the CoAP server..."
        read -n 1 -s
        run_CoAP_server
        ;;
    create-db)
        create_db
        ;;
    sql)
        query_db "$2"
        ;;
    *)
        echo "Usage: $0 {create-db|compile_all|cooja|border-router|coap-server|sim|relsim}"
        exit 1
        ;;
esac