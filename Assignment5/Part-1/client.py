import socket
import threading

# this function takes updates continuously from server
def get_updates(socket_client):

    while True:

        try:
            # takes updates upto 1024 bytes from the server and decodes using standard decoding (UTF-8)
            update = socket_client.recv(1024).decode()

            # break if no update is received
            if not update:
                break

            print(f"Document updated: {update}")

        except:
            break

def main():

    try:
        # creating socket for client with IPv4 and TCP as transport protocol
        socket_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        socket_client.connect(("127.0.0.1", 8080))
        print("Connected to the server. Start typing to edit the document:")

        # Start a thread to receive updates from the server
        threading.Thread(target=get_updates, args=(socket_client,), daemon=True).start()

        # Sending user edits to the server
        while True:
            inp = input()
            socket_client.sendall(inp.encode())

    except ConnectionRefusedError:
        print("Connection failed. Please check the server and try again")

    finally:
        socket_client.close()

if __name__ == "__main__":
    main()