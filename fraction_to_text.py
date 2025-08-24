from fractions import Fraction
from num2words import num2words

def _get_noun_plural_form(number, one, two, five):
    """
    Selects the correct plural form of a noun for a given number in Russian.
    """
    n = abs(number)
    if n % 10 == 1 and n % 100 != 11:
        return one
    if n % 10 in [2, 3, 4] and n % 100 not in [12, 13, 14]:
        return two
    return five

def convert_fraction_to_words(frac):
    """
    Converts a fractions.Fraction object to a Russian text representation
    using a simpler "X out of Y" format for the fractional part.
    """
    if not isinstance(frac, Fraction):
        if isinstance(frac, int):
            return num2words(frac, lang='ru')
        return str(frac)

    if frac.denominator == 1:
        num = frac.numerator
        whole_word = _get_noun_plural_form(num, 'целая', 'целых', 'целых')
        return f"{num2words(num, lang='ru')} {whole_word}"

    parts = []
    numerator = frac.numerator
    denominator = frac.denominator

    # Handle mixed fractions
    if numerator >= denominator:
        whole_part = numerator // denominator
        whole_text = num2words(whole_part, lang='ru')
        whole_word = _get_noun_plural_form(whole_part, 'целая', 'целых', 'целых')
        parts.append(f"{whole_text} {whole_word}")
        numerator %= denominator

    if numerator == 0:
        return " ".join(parts) if parts else "ноль"

    # --- New, simpler fractional part ---
    # Format: "[numerator] из [denominator]"
    num_word = num2words(numerator, lang='ru', gender='f') # "одна", "две"

    # Denominator needs to be in the genitive case for "из"
    denom_word = num2words(denominator, lang='ru', case='genitive')

    parts.append(f"{num_word} из {denom_word}")

    return ", ".join(parts)
