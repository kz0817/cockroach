if [ ! -d m4 ]; then
  mkdir m4
fi
touch NEWS
touch AUTHORS
touch ChangeLog
aclocal
autoreconf -i
