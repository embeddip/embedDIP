import serial
import struct
import cv2
import numpy as np
import time
from PIL import Image as PILImage  # ✅ Avoid conflict
import io

# Image format constants
IMAGE_FORMAT_GRAYSCALE = 0
IMAGE_FORMAT_RGB888 = 1
IMAGE_FORMAT_RGB565 = 2
IMAGE_FORMAT_YUV = 3
IMAGE_FORMAT_HSV = 4

# UART commands
UART_CMD_SEND_IMAGE = b"STR"
UART_CMD_RECEIVE_IMAGE = b"STW"
UART_CMD_RECEIVE_JPEG = b"STJ"

UART_BLOCK_SIZE_MAX = 65535
video_writer = None  # Global

# ✅ Renamed to avoid conflict with PIL.Image
class TransmitImage:
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
            print(f"[PC] Timeout. Padding {num_bytes - len(data)} bytes with 0x00.")
            data.extend([0x00] * (num_bytes - len(data)))
            break
        chunk = ser.read(num_bytes - len(data))
        if chunk:
            data.extend(chunk)
        else:
            time.sleep(0.01)
    return bytes(data)


def send_image_data(ser, sendImg, width, height, imageFormat):
    img = cv2.imread(sendImg)
    if img is None:
        raise FileNotFoundError(f"Image not found: {sendImg}")
    im = cv2.resize(img, (width, height))

    if imageFormat == IMAGE_FORMAT_RGB565:
        img565 = cv2.cvtColor(im, cv2.COLOR_BGR2BGR565).astype(np.uint8)
        total_bytes = width * height * 2
        img_data = img565.reshape((total_bytes,))
    elif imageFormat == IMAGE_FORMAT_GRAYSCALE:
        gray = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY).astype(np.uint8)
        total_bytes = width * height
        img_data = gray.reshape((total_bytes,))
    elif imageFormat == IMAGE_FORMAT_RGB888:
        rgb = cv2.cvtColor(im, cv2.COLOR_BGR2RGB).astype(np.uint8)
        total_bytes = width * height * 3
        img_data = rgb.reshape((total_bytes,))
    elif imageFormat == IMAGE_FORMAT_YUV:
        yuv = cv2.cvtColor(im, cv2.COLOR_BGR2YUV).astype(np.uint8)
        total_bytes = width * height * 3
        img_data = yuv.reshape((total_bytes,))
    elif imageFormat == IMAGE_FORMAT_HSV:
        hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV).astype(np.uint8)
        total_bytes = width * height * 3
        img_data = hsv.reshape((total_bytes,))
    else:
        raise ValueError(f"Unsupported format: {imageFormat}")

    offset = 0
    while offset < total_bytes:
        block_size = min(UART_BLOCK_SIZE_MAX, total_bytes - offset)
        ser.write(img_data[offset:offset + block_size])
        offset += block_size

    print("[PC] Image sent successfully.\n")


def capture_image_and_show(ser):
    print("[PC] Receiving raw image from device...")

    width = struct.unpack("<I", safe_read(ser, 4))[0]
    height = struct.unpack("<I", safe_read(ser, 4))[0]
    fmt = struct.unpack("<B", safe_read(ser, 1))[0]
    depth = struct.unpack("<B", safe_read(ser, 1))[0]

    total_size = width * height * depth
    data = bytearray()
    while len(data) < total_size:
        chunk_size = min(UART_BLOCK_SIZE_MAX, total_size - len(data))
        data += safe_read(ser, chunk_size)

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


def receive_and_show_jpeg(ser, save_path="output.avi", fps=15, resolution=(160, 120)):
    global video_writer

    # Read JPEG size
    jpeg_size = struct.unpack("<I", safe_read(ser, 4))[0]
    jpeg_data = safe_read(ser, jpeg_size, timeout=2.0)

    try:
        # Decode JPEG to RGB and convert to BGR for OpenCV
        img = PILImage.open(io.BytesIO(jpeg_data))
        img_np = np.array(img)
        img_bgr = cv2.cvtColor(img_np, cv2.COLOR_RGB2BGR)

        # Show image
        cv2.imshow("Received JPEG Image", img_bgr)
        cv2.waitKey(1)

        # Initialize video writer if not already
        if video_writer is None:
            fourcc = cv2.VideoWriter_fourcc(*'XVID')  # Or use 'MJPG'
            video_writer = cv2.VideoWriter(save_path, fourcc, fps, resolution)

        # Resize if needed (to match resolution)
        if img_bgr.shape[1::-1] != resolution:
            img_bgr = cv2.resize(img_bgr, resolution)

        # Write frame
        video_writer.write(img_bgr)

    except Exception as e:
        print(f"[ERROR] JPEG decode failed: {e}")
        return


def main():
    ser = serial.Serial("COM3", 2000000, timeout=None)
    print("[PC] Listening on COM3...")

    try:
        while True:
            header = safe_read(ser, 3)

            if header == UART_CMD_SEND_IMAGE:
                width = struct.unpack("<I", safe_read(ser, 4))[0]
                height = struct.unpack("<I", safe_read(ser, 4))[0]
                fmt = struct.unpack("<B", safe_read(ser, 1))[0]
                depth = struct.unpack("<B", safe_read(ser, 1))[0]
                print(f"[PC] STR: Device requests image {width}x{height}, fmt={fmt}, depth={depth}")
                send_image_data(ser, "bridge.png", width, height, fmt)

            elif header == UART_CMD_RECEIVE_IMAGE:
                capture_image_and_show(ser)

            elif header == UART_CMD_RECEIVE_JPEG:
                receive_and_show_jpeg(ser)

            else:
                print(f"[PC] Unknown header: {header}")
                ser.reset_input_buffer()

    except KeyboardInterrupt:
        print("\n[PC] Exiting.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
