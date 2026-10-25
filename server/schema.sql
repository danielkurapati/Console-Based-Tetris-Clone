-- Run once as a MySQL/MariaDB admin user:
--   mysql -u root -p < schema.sql
-- (the server also creates the table automatically if it is missing.)

CREATE DATABASE IF NOT EXISTS tetris CHARACTER SET utf8mb4;
USE tetris;

CREATE TABLE IF NOT EXISTS scores (
    id            INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name          VARCHAR(20)  NOT NULL,
    score         INT UNSIGNED NOT NULL,
    level         SMALLINT UNSIGNED NOT NULL,
    lines_cleared SMALLINT UNSIGNED NOT NULL,
    created_at    TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_scores_score (score)
);

-- The account the server logs in with is NOT created here, so that no password
-- is ever written in a file. Create it by hand in the mysql shell:
--   CREATE USER 'tetris'@'localhost' IDENTIFIED BY '<choose a password>';
--   GRANT SELECT, INSERT, CREATE ON tetris.* TO 'tetris'@'localhost';
