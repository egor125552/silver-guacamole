import sys
from functools import partial
from PyQt6.QtWidgets import QWidget, QVBoxLayout, QGridLayout, QLineEdit, QPushButton, QMessageBox
from PyQt6.QtCore import Qt, pyqtSignal
import re
from fractions import Fraction
from fraction_to_text import convert_fraction_to_words

class CalculatorWidget(QWidget):
    calculation_performed = pyqtSignal(str, str)

    def __init__(self):
        super().__init__()
        self.fractions_mode_active = False

        self.layout = QVBoxLayout(self)
        self.display = QLineEdit()
        self.display.setReadOnly(False) # Allow keyboard input
        self.display.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.display.setStyleSheet("font-size: 24px; padding: 10px;")
        self.display.setAccessibleName("Дисплей калькулятора")
        self.layout.addWidget(self.display)

        buttons_layout = QGridLayout()
        buttons = {
            'C': (0, 0, "Очистить"), '√': (0, 1, "Квадратный корень"), '<-': (0, 2, "Стереть"),
            '/': (1, 0, "Разделить"), '*': (1, 1, "Умножить"), '-': (1, 2, "Вычесть"),
            '+': (2, 0, "Сложить"), '=': (2, 1, 1, 2, "Равно"),
            'Режим дробей': (3, 0, 1, 3, "Режим дробей")
        }

        for text, props in buttons.items():
            button = QPushButton(text)
            button.setStyleSheet("font-size: 18px; padding: 10px;")
            button.setAccessibleName(props[-1]) # Last item is the accessible name

            if text == 'Режим дробей':
                button.setCheckable(True)
                button.toggled.connect(self.on_fractions_mode_toggled)
            else:
                button.clicked.connect(partial(self.on_button_clicked, text))

            if len(props) == 5: # For buttons with colspan/rowspan
                buttons_layout.addWidget(button, props[0], props[1], props[2], props[3])
            else:
                buttons_layout.addWidget(button, props[0], props[1])

        self.layout.addLayout(buttons_layout)

    def on_button_clicked(self, symbol):
        if symbol == 'C':
            self.display.clear()
        elif symbol == '<-':
            self.display.setText(self.display.text()[:-1])
        elif symbol == '=':
            self.evaluate_expression()
        else: # Operators and √
            self.display.setText(self.display.text() + symbol)

    def keyPressEvent(self, event):
        key = event.key()
        if key == Qt.Key.Key_Enter or key == Qt.Key.Key_Return:
            self.evaluate_expression()
        else:
            super().keyPressEvent(event)

    def evaluate_expression(self):
        try:
            original_expression = self.display.text()
            expression_to_eval = original_expression

            # Smart output detection
            # Default to simple output, only use mixed if input contained a mixed fraction
            should_convert_to_mixed = False

            # Pre-process fractions if in fractions mode
            if self.fractions_mode_active:
                # Check if the input contains a mixed fraction
                if re.search(r'(\d+),\s*(\d+)\s+(\d+)', expression_to_eval):
                    should_convert_to_mixed = True

                # Handle mixed fraction format e.g., "1, 2 3" -> "(Fraction(1,1)+Fraction(2,3))"
                expression_to_eval = re.sub(r'(\d+),\s*(\d+)\s+(\d+)', r'(Fraction(\1, 1) + Fraction(\2, \3))', expression_to_eval)
                # Handle space-separated format e.g., "2 3" -> "Fraction(2, 3)"
                expression_to_eval = re.sub(r'(\d+)\s+(\d+)', r'Fraction(\1, \2)', expression_to_eval)
                # Handle slash-separated format e.g., "2/3" -> "Fraction(2, 3)"
                expression_to_eval = re.sub(r'(\d+)/(\d+)', r'Fraction(\1, \2)', expression_to_eval)

            # NOTE: eval is not safe!
            result = eval(expression_to_eval, {"Fraction": Fraction})

            if self.fractions_mode_active:
                display_text = convert_fraction_to_words(result, convert_to_mixed=should_convert_to_mixed)
            else:
                display_text = str(result)

            self.calculation_performed.emit(original_expression, display_text)
            self.display.setText(display_text)
        except Exception as e:
            self.display.setText("Ошибка")

    def on_fractions_mode_toggled(self, checked):
        self.fractions_mode_active = checked
        if checked:
            QMessageBox.information(
                self,
                "Режим дробей",
                "Режим дробей активирован.\n\n"
                "Введите числитель, затем пробел, затем знаменатель (например, '1 2' для 1/2).\n\n"
                "Это выражение будет автоматически преобразовано в дробь при вычислении."
            )
