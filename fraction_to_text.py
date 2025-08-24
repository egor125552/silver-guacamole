from fractions import Fraction
from num2words import num2words

def _get_noun_plural_form(number, one, two, five):
    """
    Selects the correct plural form of a noun for a given number in Russian.
    'one' is for 1, 21, 31, etc. (but not 11).
    'two' is for 2-4, 22-24, etc. (but not 12-14).
    'five' is for 5-20, 25-30, etc.
    """
    n = abs(number)
    if n % 10 == 1 and n % 100 != 11:
        return one
    if n % 10 in [2, 3, 4] and n % 100 not in [12, 13, 14]:
        return two
    return five

def convert_fraction_to_words(frac):
    """
    Converts a fractions.Fraction object to a Russian text representation.
    """
    if not isinstance(frac, Fraction):
        if isinstance(frac, int):
            return num2words(frac, lang='ru')
        return str(frac)

    # If the fraction is a whole number
    if frac.denominator == 1:
        num = frac.numerator
        whole_word = _get_noun_plural_form(num, 'целая', 'целых', 'целых')
        return f"{num2words(num, lang='ru')} {whole_word}"

    parts = []
    numerator = frac.numerator
    denominator = frac.denominator

    # Handle mixed fractions (e.g., 3/2 -> 1 1/2)
    if numerator >= denominator:
        whole_part = numerator // denominator

        whole_text = num2words(whole_part, lang='ru')
        whole_word = _get_noun_plural_form(whole_part, 'целая', 'целых', 'целых')
        parts.append(f"{whole_text} {whole_word}")

        numerator %= denominator

    if numerator == 0:
        return " ".join(parts)

    # --- Fractional Part ---
    num_word = num2words(numerator, lang='ru', gender='f')

    denom_ordinal = num2words(denominator, lang='ru', to='ordinal')

    if numerator == 1:
        # Nominative singular, feminine (e.g., одна третья)
        if denom_ordinal.endswith('й'): # e.g., шестой
            denom_word = denom_ordinal[:-2] + 'ая'
        else: # e.g., третий
            denom_word = denom_ordinal[:-2] + 'ья'
    else:
        # Genitive plural (e.g., две третьих)
        if denom_ordinal.endswith('й'): # e.g., шестой
            denom_word = denom_ordinal[:-2] + 'ых'
        else: # e.g., третий
            denom_word = denom_ordinal[:-2] + 'ьих'

    parts.append(f"{num_word} {denom_word}")

    return ", ".join(parts)
