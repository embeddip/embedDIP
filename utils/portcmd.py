import serial
import struct
import time

UART_BLOCK_SIZE_MAX = 65535
UART_CMD_CAPTURE = b"STR"  # Device wants image from PC
UART_CMD_RECEIVE = b"STW"  # Device sends image to PC


class Image:
    def __init__(self, width, height, fmt, depth, pixels):
        self.width = width
        self.height = height
        self.format = fmt
        self.depth = depth
        self.pixels = pixels
        self.size = len(pixels)


def safe_read(ser, num_bytes):
    data = bytearray()
    while len(data) < num_bytes:
        chunk = ser.read(num_bytes - len(data))
        if not chunk:
            raise TimeoutError(f"Expected {num_bytes} bytes, got only {len(data)}.")
        data.extend(chunk)
    return bytes(data)


def wait_and_send_image(ser, img):
    """
    Respond to 'STR' command by sending an image to the device.
    Expects the device to first send:
      - width (uint32_t)
      - height (uint32_t)
      - format (uint32_t)
      - depth (uint32_t)
    """
    # Read metadata from device (each field is 4 bytes, little endian)
    width = struct.unpack("<I", safe_read(ser, 4))[0]
    height = struct.unpack("<I", ser.read(4))[0]
    fmt = struct.unpack("<I", ser.read(4))[0]
    depth = struct.unpack("<I", ser.read(4))[0]

    print(
        f"[PC] STR received. Device expects: {width}x{height}, Format={fmt}, Depth={depth}"
    )

    # Sanity check
    if (
        width != img.width
        or height != img.height
        or fmt != img.format
        or depth != img.depth
    ):
        print("[PC] Warning: metadata mismatch")

    # Send image pixels in blocks
    full_blocks = img.size // UART_BLOCK_SIZE_MAX
    last_block_size = img.size % UART_BLOCK_SIZE_MAX

    offset = 0
    for _ in range(full_blocks):
        ser.write(img.pixels[offset : offset + UART_BLOCK_SIZE_MAX])
        offset += UART_BLOCK_SIZE_MAX

    if last_block_size > 0:
        ser.write(img.pixels[offset : offset + last_block_size])

    print("[PC] Image sent to device.\n")


def capture_image(ser):
    width = struct.unpack("<H", ser.read(2))[0]
    height = struct.unpack("<H", ser.read(2))[0]
    fmt = struct.unpack("<B", ser.read(1))[0]

    if fmt == 1:  # RGB565
        depth = 16
    elif fmt == 3:  # Grayscale
        depth = 8
    else:
        raise ValueError(f"[PC] Unknown image format: {fmt}")

    print(
        f"[PC] STW received. Incoming image: {width}x{height}, Format={fmt}, Depth={depth}"
    )

    bytes_per_pixel = depth // 8
    size = width * height * bytes_per_pixel

    full_blocks = size // UART_BLOCK_SIZE_MAX
    last_block_size = size % UART_BLOCK_SIZE_MAX

    data = bytearray()

    for _ in range(full_blocks):
        data += ser.read(UART_BLOCK_SIZE_MAX)
    if last_block_size:
        data += ser.read(last_block_size)

    print(f"[PC] Image received from device ({len(data)} bytes).\n")
    return Image(width, height, fmt, depth, data)


def main():
    ser = serial.Serial("COM3", 2000000, timeout=None)
    print("[PC] Listening on COM3...")

    # Dummy image to send
    width, height = 480, 272
    fmt, depth = 3, 8  # grayscale
    pixels = bytes([x % 256 for x in range(width * height)])
    image_to_send = Image(width, height, fmt, depth, pixels)

    try:
        while True:
            print("[PC] Waiting for command...")
            header = ser.read(3)
            print(f"[PC] Received header bytes: {header}")
            if header == UART_CMD_CAPTURE:
                wait_and_send_image(ser, image_to_send)
            elif header == UART_CMD_RECEIVE:
                img = capture_image(ser)
                # You can save or process img here
            elif header == b"":
                print("[PC] Timeout: no command.")
            else:
                print(f"[PC] Unknown command: {header}")
    except KeyboardInterrupt:
        print("\n[PC] Stopped.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
