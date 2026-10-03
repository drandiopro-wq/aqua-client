import asyncio

ROOMS = {}  # room -> set of addr

class Proto(asyncio.DatagramProtocol):
    def datagram_received(self, data, addr):
        if len(data) < 1: return
        room_len = data[0]
        room = data[1:1+room_len].decode(errors="ignore")
        peers = ROOMS.setdefault(room, set())
        peers.add(addr)
        for p in peers:
            if p != addr:
                self.transport.sendto(data, p)

async def main():
    loop = asyncio.get_running_loop()
    await loop.create_datagram_endpoint(lambda: Proto(), local_addr=("0.0.0.0", 43770))
    print("AquaClient voice relay on :43770")
    await asyncio.Event().wait()

asyncio.run(main())
