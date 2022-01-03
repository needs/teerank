"""
Implement class Packet and class PacketException.
"""

class PacketException(Exception):
    pass

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
        data, separator, self._data = self._data.partition(b'\x00')

        if separator != b'\x00':
            raise PacketException('Cannot unpack a string.')

        return data.decode('utf-8')


    def unpack_bytes(self, n: int) -> bytes:
        """
        Unpack bytes from packet data.
        """
        if len(self._data) < n:
            raise PacketException('Cannot unpack the required amount of bytes.')

        data = self._data[:n]
        self._data = self._data[n:]
        return data


    def unpack_int(self, default: int = None) -> int:
        """Unpack an integer from packet data."""
        try:
            value = self.unpack()
            return default if value == '' else int(value)
        except ValueError as exception:
            raise PacketException('Cannot unpack an integer.') from exception


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
