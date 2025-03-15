# Use a C-based image to compile the code
FROM gcc:latest

# Install SQLite
RUN apt-get update && apt-get install -y sqlite3 libsqlite3-dev

# Create directories for server, client, and socket volume
WORKDIR /app

# Copy the server and client code
COPY ./server /app/server
COPY ./client /app/client
COPY ./files/videoteca.db /app/files/videoteca.db

# Build server and client code
RUN gcc -o /app/server/server /app/server/*.c -lsqlite3
RUN gcc -o /app/client/client /app/client/*.c -lsqlite3

CMD ["bash"]
