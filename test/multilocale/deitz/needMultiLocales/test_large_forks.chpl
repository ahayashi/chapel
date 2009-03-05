config param size: int = 256;


def main() {
  var x, y, z: size*real;

  for i in 1..size {
    x(i) = i;
  }

  writeln(x);

  on Locales(1) {
    y = x;
  }

  writeln(y);

  sync {
    on Locales(1) {
      begin {
        z = x;
      }
    }
  }

  writeln(z);
}
