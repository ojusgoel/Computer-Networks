import socket

def echo_client(hostname, port):
    # Resolving address for hostname and port
    addrinfo = socket.getaddrinfo(
        hostname, port, socket.AF_UNSPEC, socket.SOCK_STREAM
    )
    
    # Choosing the first valid address
    family, socktype, proto, canonname, sockaddr = addrinfo[0]

    # Creating the socket and connect to the server
    with socket.socket(family, socktype, proto) as client_socket:
        client_socket.connect(sockaddr)
        print(f"Connected to server at {sockaddr}")
        
        while True:
            message = input("Enter message to send (or 'exit' to quit): ")
            if message.lower() == 'exit':
                break
            client_socket.sendall(message.encode())
            data = client_socket.recv(1024)
            print(f"Echo from server: {data.decode()}")

if __name__ == "__main__":
    hostname = input("Enter the server address (e.g., 'localhost', '::1'): ")
    # port = int(input("Enter the server port (e.g., 12345): "))
    port = 8080
    echo_client(hostname, port)
