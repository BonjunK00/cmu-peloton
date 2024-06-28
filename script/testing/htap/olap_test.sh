DB_NAME="default_database"
DB_USER="postgres"
DB_HOST="localhost"
DB_PORT="15721"

SQL_FILE="olap_test.sql"

psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c "\timing on" -f $SQL_FILE