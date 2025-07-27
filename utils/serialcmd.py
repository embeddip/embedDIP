import serial
import struct
import time

UART_BLOCK_SIZE_MAX = 65535
UART_CMD_CAPTURE = b"STR"


class Image:
    def __init__(self, width, height, fmt, depth, pixels):
        self.width = width
        self.height = height
        self.format = fmt
        self.depth = depth
        self.pixels = pixels
        self.size = len(pixels)


def wait_and_send_image(ser, img):
    """
    Wait for the device to request an image, then send it.
    """
    print("[PC] Waiting for 'STR' request...")

    # Step 1: Wait for the "STR" sequence
    while True:
        sync = ser.read(3)
        if sync == UART_CMD_CAPTURE:
            print("[PC] Received 'STR' request from device.")
            break

    # Step 2: Read metadata
    width = struct.unpack("<H", ser.read(2))[0]
    height = struct.unpack("<H", ser.read(2))[0]
    fmt = struct.unpack("<H", ser.read(2))[0]
    depth = struct.unpack("<H", ser.read(2))[0]

    print(f"[PC] Metadata received: {width}x{height}, Format={fmt}, Depth={depth}")

    # Optional check (match with img metadata)
    if (
        width != img.width
        or height != img.height
        or fmt != img.format
        or depth != img.depth
    ):
        print("[PC] Warning: Metadata from device doesn't match image to send.")

    # Step 3: Send image data
    image_size = len(img.pixels)
    full_blocks = image_size // UART_BLOCK_SIZE_MAX
    last_block_size = image_size % UART_BLOCK_SIZE_MAX

    print(f"[PC] Sending image in {full_blocks} blocks + {last_block_size} bytes")

    offset = 0
    for _ in range(full_blocks):
        ser.write(img.pixels[offset : offset + UART_BLOCK_SIZE_MAX])
        offset += UART_BLOCK_SIZE_MAX

    if last_block_size > 0:
        ser.write(img.pixels[offset : offset + last_block_size])

    print("[PC] Image transfer complete.")
