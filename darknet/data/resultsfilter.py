import re
import os

# Paths to files - Use an absolute path for obj.names
input_file = 'results.txt'
output_file = 'coordinates.txt'
obj_names_file = os.path.abspath('./obj.names')  # Absolute path

# Load class names from the obj.names file and map them to indices
try:
    with open(obj_names_file, 'r', encoding='utf-8') as file:
        class_names = [line.strip() for line in file.readlines()]
        class_name_to_index = {name: index for index, name in enumerate(class_names)}
except IOError as e:
    print(f'Error reading file: {e}')
    exit(1)

# Build a regex pattern to match any class name followed by coordinates
class_names_pattern = '|'.join([re.escape(name) for name in class_names])
regex = re.compile(rf'({class_names_pattern}):\s*\d+%\s*\(left_x:\s*(-?\d+)\s*top_y:\s*(-?\d+)\s*width:\s*(\d+)\s*height:\s*(\d+)\)')

# List to store extracted coordinates
coordinates = []

# Verify if the input file exists
if not os.path.exists(input_file):
    print(f'Input file does not exist: {input_file}')
    exit(1)

# Read the results file
try:
    with open(input_file, 'r', encoding='utf-8') as file:
        content = file.read()
        print(f"Content of results.txt: {content}")  # Debugging print
        matches = regex.findall(content)
        print(f"Matches found: {matches}")  # Debugging print
        for match in matches:
            class_name, left_x, top_y, width, height = match
            class_index = class_name_to_index[class_name]  # Get the index for the class name
            coordinates.append(f'{class_index} {left_x} {top_y} {width} {height}')

except IOError as e:
    print(f'Error reading file: {e}')
    exit(1)

# Write the coordinates to the output file
try:
    with open(output_file, 'w', encoding='utf-8') as file:
        file.write('\n'.join(coordinates))

except IOError as e:
    print(f'Error writing file: {e}')
    exit(1)

print(f'Coordinates saved to {output_file}')
