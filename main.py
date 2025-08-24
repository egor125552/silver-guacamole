import sys
import os
import webbrowser
from PyQt6.QtWidgets import QApplication, QMainWindow, QMessageBox, QDockWidget, QListWidget
from PyQt6.QtGui import QAction
from PyQt6.QtCore import Qt

from calculator_widget import CalculatorWidget

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Доступный калькулятор")
        self.setGeometry(100, 100, 350, 450)

        # --- Menu Bar ---
        menu_bar = self.menuBar()

        file_menu = menu_bar.addMenu("Файл")
        exit_action = QAction("Выход", self)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)

        view_menu = menu_bar.addMenu("Вид")
        self.mixed_fraction_action = QAction("Автоматически преобразовывать в смешанные дроби", self, checkable=True)
        self.mixed_fraction_action.setChecked(True)
        view_menu.addAction(self.mixed_fraction_action)

        help_menu = menu_bar.addMenu("Справка")

        guide_action = QAction("Руководство пользователя", self)
        guide_action.triggered.connect(self.show_user_manual)
        help_menu.addAction(guide_action)

        about_action = QAction("О программе", self)
        about_action.triggered.connect(self.show_about_dialog)
        help_menu.addAction(about_action)

        # --- Central Widget ---
        self.calculator_widget = CalculatorWidget()
        self.setCentralWidget(self.calculator_widget)

        # --- History Dock ---
        history_dock = QDockWidget("История", self)
        self.history_list = QListWidget()
        history_dock.setWidget(self.history_list)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, history_dock)

        # --- Connect Signals ---
        self.mixed_fraction_action.toggled.connect(self.on_mixed_fraction_toggled)
        self.calculator_widget.calculation_performed.connect(self.add_to_history)

    def on_mixed_fraction_toggled(self, checked):
        self.calculator_widget.convert_to_mixed = checked

    def add_to_history(self, expression, result):
        self.history_list.addItem(f"{expression} = {result}")
        self.history_list.scrollToBottom()

    def show_user_manual(self):
        # Path to the manual file, assuming it's in the same directory
        manual_path = os.path.join(os.path.dirname(__file__), 'manual.html')
        if os.path.exists(manual_path):
            webbrowser.open(f'file://{os.path.realpath(manual_path)}')
        else:
            QMessageBox.warning(self, "Ошибка", "Файл руководства (manual.html) не найден.")

    def show_about_dialog(self):
        QMessageBox.about(
            self,
            "О программе",
            "<h2>Доступный калькулятор</h2>"
            "<p>Версия 1.0</p>"
            "<p>Этот калькулятор создан с учетом доступности для незрячих пользователей.</p>"
            "<p>Разработано с помощью ассистента Jules.</p>"
        )


if __name__ == "__main__":
    app = QApplication(sys.argv)
    main_win = MainWindow()
    main_win.show()
    sys.exit(app.exec())
