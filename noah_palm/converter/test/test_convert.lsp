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

(defun test-lemmas (lemmas-list-real lemmas-list-expected)
  (assert-equal (length lemmas-list-real) (length lemmas-list-expected))
  (let ((cur-exp lemmas-list-expected))
    (dolist (el-real lemmas-list-real)
      (assert-equal (car el-real) (car cur-exp))
      (setf cur-exp (cdr cur-exp))))
  )

(def-test-method test_wn-data-line->wn-synset ((ob utest))
  (let* ((line "00788865 34 v 02 put 0 assign 0 002 @ 00788109 v 0000 ~ 00789165 v 0000 01 + 21 00 | put something on or into (abstractly) assign; ; \"She put much emphasis on her the last statement\"; \"He put all his efforts into this job\"; \"The teacher put an interesting twist to the interpretation of the story\"   ")
	 (syn (wn-data-line->wn-synset line)))
    (assert-equal (wn-synset-part-of-speech syn) "v")
    (assert-equal (wn-synset-lex-filenum syn) 34)
    (test-lemmas (wn-synset-list-of-lemmas syn) '("put" "assign"))
    ))

    
(def-test-method test_2_wn-data-line->wn-synset ((ob utest))
  (let* ((line "01521645 40 v 03 trash 0 junk 0 scrap 0 001 @ 01520955 v 0000 01 + 08 00 | dispose of; \"trash these old chairs;\"; \"junk an old car\"  ")
	 (syn (wn-data-line->wn-synset line)))
    (assert-equal (wn-synset-part-of-speech syn) "v")
    (assert-equal (wn-synset-lex-filenum syn) 40)
    (test-lemmas (wn-synset-list-of-lemmas syn) '("trash" "junk" "scrap")))
  )

(def-test-method test_mismatch ((ob utest))
  (let ((test-data '(("abcd" "ab" 2) ("zdd" "re" 0) ("zdd" "" 0))))
    (dolist (one-test test-data)
      (assert-equal (third one-test) (mismatch (first one-test) (second one-test))))))

(defun test-write-num (n)
  (with-open-file
      (ostream "test.bin" :direction :output :if-exists :supersede)
    (write-int16 n ostream)))

(defun test-write ()
  (test-write-num 0))

(defun run-utests ()
  (textui-test-run (get-suite utest)))

;(run-utests)
;(load         "c:\\kjk\\src\\mine\\noah_palm\\converter\\convert.lsp")
;(setf line "01521645 40 v 03 trash 0 junk 0 scrap 0 001 @ 01520955 v 0000 01 + 08 00 | dispose of; \"trash these old chairs;\"; \"junk an old car\"  ")
;(setf syn (wn-data-line->wn-synset line))



