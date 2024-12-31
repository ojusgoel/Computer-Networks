import socket
import threading

# Managing the client
def manage_client(connection, client_addr):
    # client_addr is tuple containing client's IP addr and port number
    print(f"Connection from [{client_addr[0]}]:{client_addr[1]} (IPv6)")

    with connection:
        while True:

            # receive inp from client (upto 1024 bytes)
            inp = connection.recv(1024)
            if not inp:
                break
            print(f"Received: {inp.decode()} from [{client_addr[0]}]:{client_addr[1]}")

            # echo the data
            connection.sendall(inp)

    print(f"Connection closed for [{client_addr[0]}]:{client_addr[1]}")


def echo_server(hostname, port):

    # Creating an IPv6 socket
    with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as server_socket:

        # Allowing reuse of address and port if the server restarts quickly after shutting down.
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # Binding to the specified hostname and port for IPv6
        server_socket.bind(('', port))
        server_socket.listen(5)
        print(f"Server is listening on [{hostname}]:{port} (IPv6)")

        while True:
            # Accepting a new connection
            connection, client_addr = server_socket.accept()

            # Creating a new thread to handle the client
            client_thread = threading.Thread(target=manage_client, args=(connection, client_addr))
            #Daemonize the thread to terminate with the main program
            client_thread.daemon = True  
            client_thread.start()

if __name__ == "__main__":
    # Hostname "::" binds to all IPv6 interfaces
    hostname = "::"
    port = 8080
    echo_server(hostname, port)
