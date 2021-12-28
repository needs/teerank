class Packet:
    # Use extended packet header to support the maximum number of servers.
    HEADER = b'xe\x00\x00\x00\x00'

    def __init__(self, data: bytes = None, header: bytes = HEADER, packet_type: bytes = None):
        """Initialize packet with the given data, or with packet header and type otherwise."""
        if data is None:
            self._data = bytearray()
            self.pack_bytes(Packet.HEADER)
            self.pack_bytes(packet_type)
        else:
            self._data = data
            self.header = self.unpack_bytes(6)
            self.type = self.unpack_bytes(8)

    def unpack_remaining(self) -> int:
        """Return the number of remaining unpack() call."""
        return self._data.count(b'\x00')

    def unpack(self) -> str:
        """Unpack a string from packet data."""
        data, _, self._data = self._data.partition(b'\x00')
        return data.decode('utf-8')

    def unpack_bytes(self, n: int) -> bytes:
        """Unpack bytes from packet data."""
        data = self._data[:n]
        self._data = self._data[n:]
        return data

    def pack(self, str: str) -> None:
        """Pack a string to packet data."""
        self._data.extend(str.encode('utf-8'))

    def pack_bytes(self, data: bytes) -> None:
        """Pack bytes to packet data."""
        self._data.extend(data)

    def __len__(self):
        """Return packet length."""
        return len(self._data)

    def __bytes__(self):
        """Get packet data as bytes."""
        return bytes(self._data)
