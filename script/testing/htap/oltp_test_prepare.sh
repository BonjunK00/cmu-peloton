SYSBENCH_TEST=oltp_test.lua
SYSBENCH_OPTS="--db-driver=pgsql --pgsql-host=localhost --pgsql-port=15721 --pgsql-user=postgres --pgsql-db=default_database"

echo "Preparing sysbench test..."
sysbench $SYSBENCH_TEST $SYSBENCH_OPTS prepare