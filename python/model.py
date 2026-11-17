import torch
import torch.nn as nn


class MLP(nn.Module):
    def __init__(self, input_size=784, hidden1=64, hidden2=32, output_size=10):
        super().__init__()
        self.fc1 = nn.Linear(input_size, hidden1)   # 784 -> 64
        self.fc2 = nn.Linear(hidden1, hidden2)      # 64  -> 32
        self.fc3 = nn.Linear(hidden2, output_size)  # 32  -> 10

    def forward(self, x):
        x = x.view(x.size(0), -1)       # flatten: (batch, 1, 28, 28) -> (batch, 784)
        x = torch.relu(self.fc1(x))     # layer 1 + ReLU
        x = torch.relu(self.fc2(x))     # layer 2 + ReLU
        x = self.fc3(x)                 # output layer: raw scores, no ReLU
        return x


if __name__ == "__main__":
    model = MLP()
    print(model)

    dummy = torch.zeros(1, 1, 28, 28)   # one fake image
    print("Output shape:", model(dummy).shape)
    print("Parameters:", sum(p.numel() for p in model.parameters()))