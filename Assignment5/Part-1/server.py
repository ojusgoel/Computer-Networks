import socket
import threading
import os

# Global vars
list_clients = []
document = ""
doc_lock = threading.Lock()
client_lock = threading.Lock()

# Function to handle the clients
def handle_client(client_socket, client_addr):
    global document
    print(f"[INFO] New client connected: {client_addr}")

    # Sending the current doc state to the newly connected client
    with doc_lock:
        try:
            client_socket.sendall(document.encode())
            print(f"[INFO] Sent current document to client {client_addr}")
        except Exception as e:
            print(f"[ERROR] Failed to send document to client {client_addr}: {e}")

    try:
        # Receive updates from the client (Receives upto 1024 byte of updates from client)
        while True:
            update = client_socket.recv(1024).decode()

            # breaks the loop if client disconnects (no data received)
            if not update:
                break
            
            # Appending data to document and temp.txt
            with doc_lock:
                document += update + "\n"
                with open("temp.txt", "a") as file:
                    file.write(update + "\n")
                print(f"[UPDATE] Document updated by {client_addr}: {update.strip()}")

            # Broadcasting the update to all clients (except the sender)
            clients_snapshot = []
            with client_lock:
                clients_snapshot = list_clients[:]
            for client in clients_snapshot:
                if client != client_socket:
                    try:
                        client.sendall(update.encode())
                        print(f"[INFO] Broadcasted update to client {client.getpeername()}")
                    except Exception as e:
                        print(f"[ERROR] Failed to send update to client {client.getpeername()}: {e}")
    
    # Handling connection reset
    except ConnectionResetError:
        print(f"[INFO] Connection reset by client {client_addr}")
    
    # Handling client disconnection
    finally:
        with client_lock:
            if client_socket in list_clients:
                list_clients.remove(client_socket)

        client_socket.close()
        print(f"[INFO] Client {client_addr} disconnected")


# Function to start server
def start_server():
    global document

    # Create temp.txt if it doesn't exist
    try:
        if not os.path.exists("temp.txt"):
            open("temp.txt", "w").close()
            print("[INFO] Created temp.txt file")

    # Printing error and stopping the server        
    except Exception as e:
        print("[ERROR] Error opening file temp.txt")
        return

    # Loadign existing content from temp.txt
    try:
        with open("temp.txt", "r") as file:
            document = file.read()
        print("[INFO] Loaded existing content from temp.txt")

    except Exception as e:
        print("[ERROR] Error reading file temp.txt")
        return

    # Starting the server
    try:
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind(("0.0.0.0", 8080))
        server_socket.listen(10)
        print("[INFO] Server listening on port 8080")

        # Accepting connections
        while True:
            client_socket, client_addr = server_socket.accept()

            with client_lock:

                # Checking number of connected clients. Reject if more than 10 clients
                num_clients = 10 # Change number of allowable clients

                if len(list_clients) >= num_clients:
                    print(f"[WARN] Server full. Rejected connection from {client_addr}")
                    client_socket.sendall("Server is full. Please try again later.".encode())
                    client_socket.close()

                else:
                    list_clients.append(client_socket)
                    print(f"[INFO] Accepted connection from {client_addr}")
                    threading.Thread(target=handle_client, args=(client_socket, client_addr)).start()

    # Server Shutdown
    except Exception as e:
        print(f"[ERROR] Server error: {e}")
    
    finally:
        server_socket.close()
        print("[INFO] Server shut down")

if __name__ == "__main__":
    start_server()