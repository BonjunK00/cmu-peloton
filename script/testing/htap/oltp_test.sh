SYSBENCH_TEST=oltp_test.lua
SYSBENCH_OPTS="--db-driver=pgsql --pgsql-host=localhost --pgsql-port=15721 --pgsql-user=postgres --pgsql-db=default_database"
RUN_TIME=60
THREADS=4

echo "Running sysbench test..."
sysbench $SYSBENCH_TEST $SYSBENCH_OPTS --time=$RUN_TIME --threads=$THREADS run

echo "Cleaning up sysbench test..."
sysbench $SYSBENCH_TEST $SYSBENCH_OPTS cleanup

echo "Sysbench test completed."