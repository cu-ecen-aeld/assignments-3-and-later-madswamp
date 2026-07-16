#!/bin/sh
set -e

DAEMON="/usr/bin/aesdsocket"
DAEMON_ARGS="-d"
STOP_SIGNAL="TERM"

case "$1" in
    start)
        echo "Starting $DAEMON..."
        start-stop-daemon --start --exec "$DAEMON" -- "$DAEMON_ARGS"
        ;;
    stop)
        echo "Stopping $DAEMON..."
        start-stop-daemon --stop --exec "$DAEMON" --signal "$STOP_SIGNAL"
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

exit 0