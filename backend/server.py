import secrets

from packet import Packet

class Server:
    def __init__(self, ip: str, port: int):
        self.ip = ip
        self.port = port
        self.key = f'{ip}:{port}'
        self.address = (ip, port)
        self.token = secrets.token_bytes(nbytes=4)

    def save(self) -> None:
        pass

    def load(self) -> None:
        pass

    def start_polling(self) -> list[Packet]:
        pass

    def process_packet(self, packet: Packet) -> bool:
        pass

    def stop_polling(self) -> bool:
        pass
