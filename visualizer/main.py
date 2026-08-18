
import sys

from PySide6.QtWidgets import QApplication

from main_window import MainWindow  # TODO: read about prj style org, what if several window exists?


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.showMaximized()
    sys.exit(app.exec())
