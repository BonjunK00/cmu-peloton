SESSION="my_session"

./oltp_test_prepare.sh

tmux new-session -d -s $SESSION
tmux send-keys -t $SESSION "./oltp_test.sh" C-m

tmux split-window -h
tmux send-keys -t $SESSION "./olap_test.sh" C-m

tmux select-pane -t 0
tmux -2 attach-session -t $SESSION
