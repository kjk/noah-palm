;; unit tests for convert.lsp
(require 'clos-unit)

(load "..\\convert.lsp")

(defclass utest (test-case)
  ())

(def-test-method test_char-list->string ((ob utest))
  (assert-equal (char-list->string '(#\a #\b)) "ab")
)

(def-test-method test_is-empty-line-p ((ob utest))
  (assert-true (is-empty-line-p ""))
  (assert-true (is-empty-line-p "   "))
  (assert-true (is-empty-line-p "  "))
  (assert-true (is-empty-line-p "      "))
)

(def-test-method test_wn-data-line->wn-synset ((ob utest))
  (let* ((line "00788865 34 v 02 put 0 assign 0 002 @ 00788109 v 0000 ~ 00789165 v 0000 01 + 21 00 | put something on or into (abstractly) assign; ; \"She put much emphasis on her the last statement\"; \"He put all his efforts into this job\"; \"The teacher put an interesting twist to the interpretation of the story\"   ")
	 (syn (wn-data-line->wn-synset line)))
    (assert-equal (wn-synset-part-of-speech syn) "v")
    (assert-equal (wn-synset-lex-filenum syn) 34)
    (assert-equal 2 (length (wn-synset-list-of-lemmas syn)))
    ))

(defun run-utests ()
  (textui-test-run (get-suite utest)))

;;(run-utests)




