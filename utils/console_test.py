import serial
import struct
import cv2
import numpy as np
import time

IMAGE_FORMAT_GRAYSCALE = 0
IMAGE_FORMAT_RGB888 = 1
IMAGE_FORMAT_RGB565 = 2
IMAGE_FORMAT_YUV = 3
IMAGE_FORMAT_HSV = 4


UART_CMD_SEND_IMAGE = b"STR"
UART_CMD_RECEIVE_IMAGE = b"STW"
UART_BLOCK_SIZE_MAX = 65535


class Image:
    def __init__(self, width, height, fmt, depth, pixels):
        self.width = width
        self.height = height
        self.format = fmt
        self.depth = depth
        self.pixels = pixels
        self.size = len(pixels)


def safe_read(ser, num_bytes, timeout=0.5):
    data = bytearray()
    start_time = time.time()

    while len(data) < num_bytes:
        if time.time() - start_time > timeout:
            print(f"[PC] Timeout reached. Padding missing {num_bytes - len(data)} bytes with 0x00.")
            data.extend([0x00] * (num_bytes - len(data)))
            break

        chunk = ser.read(num_bytes - len(data))
        if chunk:
            data.extend(chunk)
        else:
            time.sleep(0.01)  # small delay to avoid busy waiting

    return bytes(data)


def send_image_data(ser, sendImg, width, height, imageFormat):
    img = cv2.imread(sendImg)
    if img is None:
        raise FileNotFoundError(f"Image not found: {sendImg}")

    im = cv2.resize(img, (width, height))

    if imageFormat == IMAGE_FORMAT_RGB565:
        img565 = cv2.cvtColor(im, cv2.COLOR_BGR2BGR565)
        img565 = img565.astype(np.uint8)
        total_bytes = width * height * 2
        img_data = img565.reshape((total_bytes,))
        print(f"[PC] Format: RGB565 — {total_bytes} bytes to send.")

    elif imageFormat == IMAGE_FORMAT_GRAYSCALE:
        gray = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)
        gray = gray.astype(np.uint8)
        total_bytes = width * height
        img_data = gray.reshape((total_bytes,))
        print(f"[PC] Format: Grayscale — {total_bytes} bytes to send.")

    elif imageFormat == IMAGE_FORMAT_RGB888:
        rgb = cv2.cvtColor(im, cv2.COLOR_BGR2RGB)
        rgb = rgb.astype(np.uint8)
        total_bytes = width * height * 3
        img_data = rgb.reshape((total_bytes,))
        print(f"[PC] Format: RGB888 — {total_bytes} bytes to send.")

    elif imageFormat == IMAGE_FORMAT_YUV:
        yuv = cv2.cvtColor(im, cv2.COLOR_BGR2YUV)
        yuv = yuv.astype(np.uint8)
        total_bytes = width * height * 3
        img_data = yuv.reshape((total_bytes,))
        print(f"[PC] Format: YUV — {total_bytes} bytes to send.")

    elif imageFormat == IMAGE_FORMAT_HSV:
        hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)
        hsv = hsv.astype(np.uint8)
        total_bytes = width * height * 3
        img_data = hsv.reshape((total_bytes,))
        print(f"[PC] Format: HSV — {total_bytes} bytes to send.")

    else:
        raise ValueError(f"Unsupported format: {imageFormat}")

    # Send in blocks
    offset = 0
    full_blocks = total_bytes // UART_BLOCK_SIZE_MAX
    last_block = total_bytes % UART_BLOCK_SIZE_MAX

    for _ in range(full_blocks):
        ser.write(img_data[offset : offset + UART_BLOCK_SIZE_MAX])
        offset += UART_BLOCK_SIZE_MAX

    if last_block > 0:
        ser.write(img_data[offset : offset + last_block])

    print("[PC] Image sent successfully.\n")


def capture_image_and_show(ser):
    print("[PC] Receiving image from device...")

    width = struct.unpack("<I", safe_read(ser, 4))[0]
    height = struct.unpack("<I", safe_read(ser, 4))[0]
    fmt = struct.unpack("<B", safe_read(ser, 1))[0]
    depth = struct.unpack("<B", safe_read(ser, 1))[0]

    print(f"[PC] Metadata received: {width}x{height}, Format={fmt}, Depth={depth}")

    total_size = width * height * depth
    full_blocks = total_size // UART_BLOCK_SIZE_MAX
    last_block = total_size % UART_BLOCK_SIZE_MAX

    data = bytearray()
    for _ in range(full_blocks):
        data += safe_read(ser, UART_BLOCK_SIZE_MAX)
    if last_block > 0:
        data += safe_read(ser, last_block)

    print(f"[PC] Received {len(data)} bytes.\n")

    # Decode and show image
    if fmt == IMAGE_FORMAT_RGB565:
        img565 = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 2))
        img_bgr = cv2.cvtColor(img565, cv2.COLOR_BGR5652BGR)
        cv2.imshow("Received RGB565 Image", img_bgr)

    elif fmt == IMAGE_FORMAT_GRAYSCALE:
        img_gray = np.frombuffer(data, dtype=np.uint8).reshape((height, width))
        cv2.imshow("Received Grayscale Image", img_gray)

    elif fmt == IMAGE_FORMAT_RGB888:
        img_rgb = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 3))
        cv2.imshow("Received RGB888 Image", img_rgb)

    elif fmt == IMAGE_FORMAT_YUV:
        img_yuv = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 3))
        img_bgr = cv2.cvtColor(img_yuv, cv2.COLOR_YUV2BGR)
        cv2.imshow("Received YUV Image", img_bgr)

    elif fmt == IMAGE_FORMAT_HSV:
        img_hsv = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 3))
        img_bgr = cv2.cvtColor(img_hsv, cv2.COLOR_HSV2BGR)
        cv2.imshow("Received HSV Image", img_bgr)

    else:
        print(f"[PC] Unsupported format: {fmt}")
        return

    cv2.waitKey(0)
    cv2.destroyAllWindows()


def main():
    ser = serial.Serial("COM3", 2000000, timeout=None)
    print("[PC] Listening on COM3...")

    try:
        while True:
            header = safe_read(ser, 3)
            print(f"[PC] Header received: {header}")

            if header == UART_CMD_SEND_IMAGE:
                # Receive metadata
                width = struct.unpack("<I", safe_read(ser, 4))[0]
                height = struct.unpack("<I", safe_read(ser, 4))[0]
                fmt = struct.unpack("<B", safe_read(ser, 1))[0]
                depth = struct.unpack("<B", safe_read(ser, 1))[0]

                print(
                    f"[PC] STR: Device requests image {width}x{height}, fmt={fmt}, depth={depth}"
                )
                send_image_data(ser, "bridge.png", width, height, fmt)

            elif header == UART_CMD_RECEIVE_IMAGE:
                capture_image_and_show(ser)

            else:
                print(f"[PC] Unknown header: {header}")
    except KeyboardInterrupt:
        print("\n[PC] Exiting.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
