# AI Usage Policy

This document outlines rules for AI (LLM) usage when interacting with the RmlUi GitHub repository.

The rules are intended to contribute to a healthy community, and to ensure that the library can continue to develop in a sustainable way. In particular, they are not meant as a judgment about AI usage.

## Communication on GitHub is for humans

All *communication* on the RmlUi GitHub repository must be written by human authors. AI is not allowed. 

In particular, this applies to:

- Issues.
- Pull request descriptions, and comments.
- Discussion posts.

Exceptions:

- Translation. AI must translate text as authentically to the original text as possible, never add its own style or polish.
- Quotation. Quoting from an AI in a post is allowed, as long as:
  - It is attributed in the same way you would another human.
  - Proper context is given in your own words.
  - Most of the post is not AI, i.e. no long AI dumped output.

No other exceptions.

### *Motivation*

The first aspect is purely practical: Humans are much better than AI at giving the right context, and doing so in a succinct way. This moves some of the effort from the readers back to the author, which contributes to a more sustainable development.

The second aspect is about community: Communicating with other human beings is more fulfilling than with AI, and the only way to foster a healthy community.

Keep in mind that this library is developed purely on a volunteer-basis, given away for free, and with no commercial aspect tied to it. Several people give away their own free time for this project. Keeping it motivating and fulfilling for everyone involved is essential to the health of the library.

## Pull requests

### Large AI authored pull requests

Large pull requests where code is mostly or fully authored by AI are not accepted.

### Other pull requests

Using AI for smaller pull requests, or in smaller parts of larger pull requests, are accepted under the following guidelines:

- Usage of AI must be disclosed, and should include:
  - To what extent AI was used, and which model was used.
  - Description of which parts are mostly or fully AI written.
- The author is responsible for the code produced by AI.
- The author must fully understand the code they submit.
- The author has tested and reviewed their own code first.
- The author is certain it works correctly, also in other environments than their own.

Remember that all communication must be written by humans, as outlined above.

### *Motivation*

Reviewing AI code takes a considerable amount of time. Often a lot more than it took to write it originally. This causes an imbalance, where the effort is moved from the author over to reviewers and the maintainer. Since the community has limited resources, it is simply not sustainable to keep up with the volume of fully AI authored pull requests. Thus, we have to make some restrictions.

With AI it is challenging to evaluate how much effort the author has put into the changes, and how much effort they are willing to follow up with it. How important is the change to them? By requiring some manual involvement, we at least ensure some balance of efforts between the author and reviewers.

To underscore this aspect, we have seen AI pull requests and issues whose author were not even aware of them, whose agent had decided on its own to submit them without even involving the author. Some of these issues being back-and-forth deliberations in the AI's own reasoning. This produces noise, and wastes everyone's time.
