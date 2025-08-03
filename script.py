for i in range(0, 100):
  with open(f"large-folder/file{i}", "w") as f:
    f.write(f"Hello {i}")