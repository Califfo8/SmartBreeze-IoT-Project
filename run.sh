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

function compile_node(){
    local node_name=$1
    local target="nrf52840"

    cd ./$node_name
    make distclean
    make $TARGET_BOARD $node_name
    cd ..
}

function compile_all_nodes(){
    echo "Inizio compilazione:"
    for node in $NODE_LIST
    do
        compile_node $node
        echo -e "\t - ${node} compilato\n"
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
        gnome-terminal -- bash -c ' cd ..;cd rpl-border-router;make TARGET=nrf52840 BOARD=dongle connect-router;'
        echo "Connecting rpl-border-router to dongle"
    else
        gnome-terminal -- bash -c ' cd ..;cd rpl-border-router;make TARGET=cooja connect-router-cooja;'
        echo "Connecting rpl-border-router to cooja"
    fi
    
}

function run_CoAP_server(){
    gnome-terminal -- bash -c 'cd ./PythonApplication; python3 ./server.py; exec bash'
}

function run_user_app(){
    gnome-terminal -- bash -c 'cd ./PythonApplication; python3 ./userapp.py; exec bash'
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
    settings VARCHAR(255) NOT NULL,
    PRIMARY KEY (ip));"

    mysql_cmd "CREATE TABLE IF NOT EXISTS ${DATABASE_NAME}.solar_production 
    (timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    sampled FLOAT DEFAULT NULL, 
    predicted FLOAT DEFAULT NULL,
    PRIMARY KEY (timestamp));"

    mysql_cmd "CREATE TABLE IF NOT EXISTS ${DATABASE_NAME}.temperature 
    (timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    temperature FLOAT NOT NULL,
    active_HVAC INT NOT NULL,
    PRIMARY KEY (timestamp));"
    echo "Database creato"
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
        cd ./$node_name
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
        create_db
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
        create_db
        run_rpl_border_router "cooja"
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
    user)
        run_user_app
        ;;
    flash)
        flash
        ;;
    deploy)
        create_db
        run_rpl_border_router
        echo "Press any key to start the CoAP server..."
        read -n 1 -s
        run_CoAP_server
        run_user_app
        ;;
    *)
        echo "Usage: $0 {create-db|compile_all|cooja|border-router|coap-server|sim|relsim}"
        exit 1
        ;;
esac