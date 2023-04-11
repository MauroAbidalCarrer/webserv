const fileInput = document.querySelector('input[type="file"]');

fileInput.addEventListener('change', async (event) => {
  const file = event.target.files[0];

  const response = await fetch('https://example.com/upload', {
    method: 'POST',
    headers: {
      'Content-Type': 'video/mp4',
      'Transfer-Encoding': 'chunked'
    },
    body: file
  });

  console.log(`Response status: ${response.status}`);
});