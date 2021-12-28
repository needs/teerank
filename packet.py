"""
Implement class Packet.
"""

class Packet:
    """
    Implement an easy way of packing and unpacking data according to the
    Teeworlds packet data format.
    """

    def __init__(self, data: bytearray = None):
        """
        Initialize packet with the given data.
        """
        if data is None:
            data = bytearray()

        self._data = data

    def unpack_remaining(self) -> int:
        """
        Return the number of remaining unpack() call.
        """
        return self._data.count(b'\x00')

    def unpack(self) -> str:
        """
        Unpack a string from packet data.
        """
        data, _, self._data = self._data.partition(b'\x00')
        return data.decode('utf-8')

    def unpack_bytes(self, n: int) -> bytes:
        """
        Unpack bytes from packet data.
        """
        data = self._data[:n]
        self._data = self._data[n:]
        return data

    def pack(self, str: str) -> None:
        """
        Pack a string to packet data.
        """
        self._data.extend(str.encode('utf-8'))

    def pack_bytes(self, data: bytes) -> None:
        """
        Pack bytes to packet data.
        """
        self._data.extend(data)

    def __len__(self) -> int:
        """
        Return the number of bytes in the packet.
        """
        return len(self._data)

    def __bytes__(self) -> bytes:
        """
        Get packet data as bytes.
        """
        return bytes(self._data)
