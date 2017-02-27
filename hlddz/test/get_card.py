#!/usr/bin/python
# -*- coding: utf-8 -*-

face_symbols = ['2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A']
suit_symbols = ['d', 'c', 'h', 's']


class Card(object):
    ''''''
    def __init__(self):
        ''''''
        self.face = 0
        self.suit = 0
        self.value = 0

    def set_value(self, val):
        ''''''
        self.value = val
        self.face = self.value & 0xF
        self.suit = self.value >> 4
        if self.face < 2:
            self.face += 13

    def get_card(self):
        ''''''
        str_card = '%s%s' % (face_symbols[self.face - 2], suit_symbols[self.suit])
        return str_card


card_list = [33, 27, 51, 45, 42, 38]
card_result = []

for val in card_list:
    print val
    card = Card()
    card.set_value(val)
    card_result.append(card.get_card())


print card_result




