import torch
import torch.nn as nn
from torch.utils.data import DataLoader
from torchvision import datasets, transforms
from pathlib import Path

from model import MLP                          # your model from model.py

DATA_DIR = Path(__file__).parent / "data"

# --- data ---
train_set = datasets.MNIST(root=DATA_DIR, train=True,  download=True, transform=transforms.ToTensor())
test_set  = datasets.MNIST(root=DATA_DIR, train=False, download=True, transform=transforms.ToTensor())
train_loader = DataLoader(train_set, batch_size=64, shuffle=True)   # 64 images per step, new order each epoch
test_loader  = DataLoader(test_set,  batch_size=256)                # bigger batches: we only evaluate here

# --- model, loss, optimizer ---
torch.manual_seed(0)                           # same random start every run -> reproducible results
model = MLP()
loss_fn = nn.CrossEntropyLoss()                # "how wrong" for classification
optimizer = torch.optim.Adam(model.parameters(), lr=1e-3)

# --- training ---
for epoch in range(5):
    model.train()
    running_loss = 0.0
    for images, labels in train_loader:
        outputs = model(images)                # 1. forward: let the model guess
        loss = loss_fn(outputs, labels)        # 2. how wrong was it?
        optimizer.zero_grad()                  # 3a. clear old gradients
        loss.backward()                        # 3b. compute how each weight affects the loss
        optimizer.step()                       # 4. adjust the weights a little
        running_loss += loss.item()

    # --- evaluation on the test set ---
    model.eval()
    correct = 0
    with torch.no_grad():                      # no learning here, just checking
        for images, labels in test_loader:
            outputs = model(images)
            predicted = outputs.argmax(dim=1)  # index of the largest score = the guessed digit
            correct += (predicted == labels).sum().item()

    avg_loss = running_loss / len(train_loader)
    print(f"epoch {epoch + 1}: train loss {avg_loss:.4f}, test accuracy {100 * correct / len(test_set):.2f} %")

# --- save the trained weights ---
torch.save(model.state_dict(), Path(__file__).parent / "mlp_float.pt")
print("saved mlp_float.pt")