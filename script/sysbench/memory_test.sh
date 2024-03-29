PROCESS_NAME="peloton"
OUTPUT_FILE="memory_usage.txt"
INTERVAL=5

echo "Timestamp, RSS (KB)" > $OUTPUT_FILE

while true; do
    TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")
    MEMORY_USAGE=$(ps aux | grep $PROCESS_NAME | grep -v grep | awk '{print $6}')
    
    if [ ! -z "$MEMORY_USAGE" ]; then
        echo "$TIMESTAMP, $MEMORY_USAGE" >> $OUTPUT_FILE
    fi
    
    sleep $INTERVAL
done
