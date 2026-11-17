import torch
from torchvision import datasets, transforms
from pathlib import Path

# Always use python/data next to this script, no matter where the terminal is
DATA_DIR = Path(__file__).parent / "data"

# ToTensor() turns each image into a tensor with values from 0.0 to 1.0
train_set = datasets.MNIST(root=DATA_DIR, train=True,  download=True, transform=transforms.ToTensor())
test_set  = datasets.MNIST(root=DATA_DIR, train=False, download=True, transform=transforms.ToTensor())

# 1. How many images?
print("Training images:", len(train_set))
print("Test images:    ", len(test_set))

# 2. Look at the first training image
image, label = train_set[0]
print("Shape:", image.shape)          # (channels, height, width)
print("Label:", label)                # the digit this image shows
print("Min:", image.min().item(), "Max:", image.max().item())

# 3. Draw it as text: '#' for dark ink, '.' for background
for row in range(28):
    line = ""
    for col in range(28):
        if image[0, row, col] > 0.5:
            line += "#"
        else:
            line += "."
    print(line)

# 4. What the MLP will see: the 28x28 image flattened into one row of 784 numbers
flat = image.view(-1)
print("Flattened shape:", flat.shape)                       # torch.Size([784])
print("Row 14 of the image = x[392] to x[419]:")
print([round(v, 2) for v in flat[14*28 : 15*28].tolist()])  # one row of the digit, as numbers