-- create database for supporting iNoah for Palm

DROP DATABASE IF EXISTS inoahdb;
CREATE DATABASE inoahdb;
USE inoahdb;

CREATE TABLE words
(
  word_no    INT,
  word       VARCHAR(255) BINARY NOT NULL,
  word_snd   VARCHAR(255) BINARY, -- soundex version of a word
  pron       VARCHAR(255) BINARY NOT NULL,
  def        TEXT NOT NULL,

  PRIMARY KEY(word)
);

CREATE TABLE request_log
(
  cookie     VARCHAR(64)  NOT NULL, -- 64 just to be sure
  word       VARCHAR(255) BINARY NOT NULL,
  query_time TIMESTAMP NOT NULL,

  INDEX idx1 (cookie)
);

CREATE TABLE cookies
(
  cookie        VARCHAR(64)   NOT NULL, -- unique cookie that we generate
  dev_info      TEXT NOT NULL,          -- we get it from the client
  reg_code      VARCHAR(64) NULL,       -- registration code, NULL if not registered
  when_created  TIMESTAMP NOT NULL,     -- when this cookie has been created
  disabled_p    CHAR(1) NOT NULL DEFAULT 'f', -- boolean value, is this cookie disabled.
                                            -- redundant with reg_codes.disabled_p for speed
                                            -- 'f' means disabled, 'true' means not disabled
  PRIMARY KEY(cookie)
);

-- table contains list of valid registration codes
CREATE TABLE reg_codes
(
  reg_code      VARCHAR(64) NOT NULL,
  user_name     VARCHAR(255),
  user_email    VARCHAR(255),
  order_id      VARCHAR(255),
  when_entered  TIMESTAMP NOT NULL,
  disabled_p    CHAR(1) NOT NULL DEFAULT 'f',  -- is this reg code disabled (e.g. because we decided people shared it)

  PRIMARY KEY (reg_code)
);

-- registration code for testing
--INSERT INTO reg_codes VALUES ( 'beh', 'test user', 'test@user.com', 'test order id', CURRENT_TIMESTAMP(), 'f' );

-- another registration code, this time disabled
--INSERT INTO reg_codes VALUES ( 'disabled', 'disabled test user', 'disabled_test@user.com', 'disabled test order id', CURRENT_TIMESTAMP(), 't');

--INSERT INTO cookies VALUES ( 'berake', 'HNaabbcc', 'beh', CURRENT_TIMESTAMP(), 'f' );

-- INSERT INTO request_log ($cookie, $word, CURRENT_TIMESTAMP());

