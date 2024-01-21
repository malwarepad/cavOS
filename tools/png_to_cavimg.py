import sys
from PIL import Image
import numpy as np

def resize_image(input_path, output_path, target_width, target_height):
    with Image.open(input_path) as img:
        original_width, original_height = img.size
        ratio = min(target_width / original_width, target_height / original_height)
        new_size = (int(original_width * ratio), int(original_height * ratio))
        img_resized = img.resize(new_size, Image.Resampling.LANCZOS)
        new_img = Image.new("RGB", (target_width, target_height), (0, 0, 0))
        new_img.paste(img_resized, ((target_width - new_size[0]) // 2, (target_height - new_size[1]) // 2))
        rgb_values = np.array(new_img)
        hex_values = bytearray(rgb_values.flatten())
        with open(output_path, 'wb') as file:
            file.write(hex_values)
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python png_to_cavimg <input_png_file> <output_hex_file>")
        sys.exit(1)
    input_file = sys.argv[1]
    output_file = sys.argv[2]

    resize_image(input_file, output_file, 800, 600)