
import sys

from PySide6.QtWidgets import QApplication

from mainwindow import MainWindow  # TODO: read about prj style org, what if several window exists?


if __name__ == "__main__":
    app = QApplication(sys.argv)  # TODO: check Qt window types
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
