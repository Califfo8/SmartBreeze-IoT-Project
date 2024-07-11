#---------------------------------------------
#               PARAMETERS
#---------------------------------------------
# List of nodes to compile
NODE_LIST="energy-manager climate-manager"
# MySQL Credentials for creating the database
MYSQL_USER="root"
MYSQL_PASSWORD="PASSWORD"

#---------------------------------------------

#Utility variables
DATABASE_NAME="SmartBreezeDB"
TARGET_BOARD="TARGET=nrf52840 BOARD=dongle"
BASE_PATH="Implementation/"

function compile_node(){
    local node_name=$1
    local target="nrf52840"
    local actual_path=$(pwd)

    cd ./$BASE_PATH$node_name
    make distclean 2>&1 | grep -E "error|warning|TARGET not defined, using target 'native'" | grep -v "CC "
    make $TARGET_BOARD $node_name 2>&1 | grep -E "error|warning|TARGET not defined, using target 'native'" | grep -v "CC "
    cd "$actual_path"
}

function compile_all_nodes(){
    echo "Start compaling all the nodes:"
    for node in $NODE_LIST
    do
        echo -e "\t - Compiling ${node}..."
        compile_node $node
    done
    echo "Fine compilazione"
}

function run_cooja(){
    #./gradlew run --info --debug
    gnome-terminal -- bash -c 'cd; cd contiki-ng/tools/cooja; ./gradlew run; exec bash'
}

function run_rpl_border_router(){
    local target=$1
    if [ "$target" != "cooja" ]; then
        gnome-terminal -- bash -c ' cd ..;cd rpl-border-router;make '$TARGET_BOARD' connect-router;'
        echo "Connecting rpl-border-router to dongle"
    else
        gnome-terminal -- bash -c ' cd ..;cd rpl-border-router;make TARGET=cooja connect-router-cooja;'
        echo "Connecting rpl-border-router to cooja"
    fi
    
}

function run_CoAP_server(){

    gnome-terminal -- bash -c 'cd ./'$BASE_PATH'PythonApplication; python3 ./server.py; exec bash'
}

function run_user_app(){
    gnome-terminal -- bash -c 'cd ./'$BASE_PATH'PythonApplication; python3 ./userapp.py; exec bash'
}

function mysql_cmd() {
    mysql -u$MYSQL_USER -p$MYSQL_PASSWORD -e "$1" 2>&1 | grep -v "Warning: Using a password"
}

function create_db(){
    local fresh_start=$1
    # Create database
    if [ "$fresh_start" == "fresh" ]; then
        echo "Deleting previous database..."
        mysql_cmd "DROP DATABASE IF EXISTS ${DATABASE_NAME};" > /dev/null
    fi
    echo "Creating new database if not exists..."
    mysql_cmd "CREATE DATABASE IF NOT EXISTS ${DATABASE_NAME};" > /dev/null

    #Adding tables
    mysql_cmd "CREATE TABLE IF NOT EXISTS ${DATABASE_NAME}.nodes 
    (ip VARCHAR(50) NOT NULL,
    name VARCHAR(255) NOT NULL, 
    resource VARCHAR(255) NOT NULL,
    settings VARCHAR(255) NOT NULL,
    PRIMARY KEY (ip));" > /dev/null
    echo -e "\t- \"nodes\" table created"

    mysql_cmd "CREATE TABLE IF NOT EXISTS ${DATABASE_NAME}.solar_production 
    (timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    sampled FLOAT DEFAULT NULL, 
    predicted FLOAT DEFAULT NULL,
    PRIMARY KEY (timestamp));" > /dev/null
    echo -e "\t- \"solar_production\" table created"

    mysql_cmd "CREATE TABLE IF NOT EXISTS ${DATABASE_NAME}.temperature 
    (timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    temperature FLOAT NOT NULL,
    active_HVAC INT NOT NULL,
    PRIMARY KEY (timestamp));"> /dev/null
    echo -e "\t- \"temperature\" table created"

    echo "Database created successfully!"
}

function query_db()
{
    mysql -u$MYSQL_USER -p$MYSQL_PASSWORD -D$DATABASE_NAME -e "$1"
}

function flash_rpl_border_router() {
    echo "Flashing RPL border router..."
    cd ..
    cd rpl-border-router
    make $TARGET_BOARD PORT=/dev/ttyACM0 border-router.dfu-upload
    cd - > /dev/null
}

# Function to flash a sensor
function flash_sensor() {
    local node_name=$1
    
    if [ -n "$node_name" ]; then
        echo "Flashing $node_name sensor..."
        cd ./$BASE_PATH$node_name
        #make distclean
        make $TARGET_BOARD ${node_name}.dfu-upload PORT=/dev/ttyACM0
        cd - > /dev/null
    else
        echo "Invalid sensor name: $node_name"
    fi
}

flash_all() {
    for node_name in $NODE_LIST; do
        read -p "Press Enter to flash the next sensor (${node_name})..."
        flash_sensor $node_name
    done
    read -p "Press Enter to flash the next sensor (rpl border router)..."
    flash_rpl_border_router
}

function flash()
{
    # Prompt the user for the number of sensors
    echo "How many sensors do you want to flash? (Enter 'all' to flash all sensors)"
    read num_sensors
    if [ "$num_sensors" == "all" ]; then
        flash_all
    else
        for ((i=1; i<=$num_sensors; i++)); do
            read -p "Enter the name of the sensor to flash: " sensor_name
            if [ $sensor_name == "rpl-border-router" ]; then
                flash_rpl_border_router
                continue
            else
                flash_sensor $sensor_name
            fi
        done
    fi
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
        run_rpl_border_router $2
        ;;
    coap-server)
        run_CoAP_server
        ;;
    sim)
        create_db "fresh"
        compile_all_nodes
        echo "Starting Cooja..."
        run_cooja
        echo "Press any key to start the border-router..."
        read -n 1 -s
        run_rpl_border_router "cooja"
        echo "Press any key to start the CoAP server..."
        read -n 1 -s
        run_CoAP_server
        ;;
    relsim)
        create_db "fresh"
        run_rpl_border_router "cooja"
        echo "Press any key to start the CoAP server..."
        read -n 1 -s
        run_CoAP_server
        ;;
    create-db)
        create_db "$2"
        ;;
    sql)
        query_db "$2"
        ;;
    user)
        run_user_app
        ;;
    flash)
        flash
        ;;
    deploy)
        echo "Starting deployment..."
        create_db
        run_rpl_border_router
        echo "Press any key to start the CoAP server..."
        read -n 1 -s
        run_CoAP_server
        run_user_app
        ;;
    *)
        echo "------------------------------------------------------------------HELP----------------------------------------------------------------"
        echo "Usage: $0 {compile_all|compile|cooja|border-router|coap-server|sim|relsim|create-db|sql|user|flash|deploy}"
        echo "----------------------------------------------------------------COMMANDS--------------------------------------------------------------"
        echo "[1] WRAP-UP COMMANDS:"
        echo -e "\t-sim: Resets the database, compiles the nodes, runs the Cooja simulator, connects the border router and starts the CoAP server"
        echo -e "\t-relsim: Runs the coap server and connects the border router to cooja"
        echo -e "\t-flash: Flashes the nodes to the dongles"
        echo -e "\t-deploy: Deploys the system"
        echo "[2] SINGLE COMMANDS:"
        echo -e "\t-compile_all: Compiles all the nodes"
        echo -e "\t-compile: Compiles a specific node. Usage: compile <node_name>"
        echo -e "\t-cooja: Runs the Cooja simulator"
        echo -e "\t-border-router: Runs the RPL border router. Usage: border-router <target>"
        echo -e "\t-coap-server: Runs the CoAP server"
        echo -e "\t-create-db: Creates the database if not exists. Write 'fresh' to delete the previous database. Usage: create-db <fresh>"
        echo -e "\t-sql: Executes a SQL query on the database. Usage: sql <query>"
        echo -e "\t-user: Runs the user application"

        exit 1
        ;;
esac