#!/usr/bin/env python3
import socket, threading, sys

LISTEN_HOST = '127.0.0.1'
LISTEN_PORT = 502

REMOTE_HOST = '10.0.67.199'
REMOTE_PORT = 502

BUFFER_SIZE = 4096

def forward(src, dst, direction):
    try:
        while True:
            data = src.recv(BUFFER_SIZE)
            if not data:
                print(f"[{direction}] conexão fechada (0 bytes).", flush=True)
                break
            dst.sendall(data)
            preview = data[:16].hex()
            print(f"[{direction}] {len(data)} bytes | preview: {preview}…", flush=True)
    except Exception as e:
        print(f"[{direction}] erro: {e}", file=sys.stderr, flush=True)
    finally:
        src.close(); dst.close()

def handle_client(client_sock, client_addr):
    print(f"[+] Novo cliente: {client_addr}", flush=True)
    try:
        remote_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        remote_sock.connect((REMOTE_HOST, REMOTE_PORT))
        print(f"[+] Conectado ao remoto {REMOTE_HOST}:{REMOTE_PORT}", flush=True)
    except Exception as e:
        print(f"[!] Falha ao conectar remoto → {e}", file=sys.stderr, flush=True)
        client_sock.close()
        return

    threading.Thread(target=forward, args=(client_sock, remote_sock, "C→R"), daemon=True).start()
    threading.Thread(target=forward, args=(remote_sock, client_sock, "R→C"), daemon=True).start()

def main():
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        listener.bind((LISTEN_HOST, LISTEN_PORT))
    except OSError as e:
        print(f"[!] Não foi possível bindar em {LISTEN_HOST}:{LISTEN_PORT} → {e}", file=sys.stderr, flush=True)
        sys.exit(1)

    listener.listen(5)
    print(f"[+] Proxy escutando em {LISTEN_HOST}:{LISTEN_PORT} → {REMOTE_HOST}:{REMOTE_PORT}", flush=True)

    try:
        while True:
            client_sock, client_addr = listener.accept()
            handle_client(client_sock, client_addr)
    except KeyboardInterrupt:
        print("\n[!] Encerrando proxy...", flush=True)
    finally:
        listener.close()

if __name__ == '__main__':
    main()