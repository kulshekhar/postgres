-- Neither disk space nor time needs to be wasted.
--
-- Here the tests are designed for segments of multiple buffers.
SET buffile_max_filesize TO 32768;

SELECT buffile_test_shared();
SELECT buffile_test_shared_append();
